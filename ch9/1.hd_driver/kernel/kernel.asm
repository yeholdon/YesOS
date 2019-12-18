
; 内核段选择子CS=Core Segment，TI=RPL=0
;SELECTOR_KERNEL_CS  equ 8			;已经放入sconst.inc
%include	"sconst.inc"

; 导入函数调用别人的，c语言函数
extern  cstart
extern	exception_handler
extern	spurious_irq
extern	disp_str

extern	kernel_main						;	// 内核主函数
extern	clock_handler

extern	irq_table
; 导入全局变量
extern  gdt_ptr
extern	idt_ptr
extern	disp_pos		;这个定义在kliba.asm中
extern	p_proc_ready
extern	tss
extern	disp_pos
extern	k_reenter		;防止时钟中断嵌套造成问题
extern	sys_call_table

; 中断例程使用
[SECTION .data]
clock_int_msg		db	"^", 0

[SECTION .bss]      ; Block Started by Symbol
StackSpace	resb	2*1024
StackTop:

[SECTION .text]		; 代码
global _start	;导出_start让链接器知道入口

global	restart
global	sys_call

; 导出这些中断处理函数名，以供C调用
global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
;中断处理程序的函数名导出，C语言里设置描述符的时候要用到它们的地址
global  hwint00
global  hwint01
global  hwint02
global  hwint03
global  hwint04
global  hwint05
global  hwint06
global  hwint07
global  hwint08
global  hwint09
global  hwint10
global  hwint11
global  hwint12
global  hwint13
global  hwint14
global  hwint15
_start:
	;主要实现堆栈的切换（ESP的修改）、GDT复制到内核并重新lgdt和更新高速缓存
	; GDT 以及相应的描述符是这样的：
	;
	;		              Descriptors               Selectors
	;              ┏━━━━━━━━━━━━━━━━━━┓
	;              ┃         Dummy Descriptor           ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_C    (0～4G)     ┃   8h = cs
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_RW   (0～4G)     ┃  10h = ds, es, fs, ss
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_VIDEO                 ┃  1Bh = gs
	;              ┗━━━━━━━━━━━━━━━━━━┛
	;
	; 注意! 在使用 C 代码的时候一定要保证 ds, es, ss 这几个段寄存器的值是一样的
	; 因为编译器有可能编译出使用它们的代码, 而编译器默认它们是一样的. 比如串拷贝操作会用到 ds 和 es.
	;
	;

	; 切换堆栈，修改esp
	mov	esp, StackTop		; 堆栈在bss段中

	mov	dword [disp_pos], 0
	;更新gdt的地址
	sgdt	[gdt_ptr]				;保存gdt所在的段地址和偏移地址内存	
	call	cstart						  ;cstart()函数改变gdt_ptr使其指向内核里的gdt
	lgdt	[gdt_ptr]			

	lidt	[idt_ptr]					; 加载idt的地址进idtr

	jmp SELECTOR_KERNEL_CS:csinit	;“这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.
																			;更新了高速缓存中的内容

csinit:
	;push	0		;清空标准寄存器
	;popfd			; Pop top of stack into EFLAGS
	;jmp	0x40:0
	;sti					;置位IF打开中断
	;hlt					;切换工作完成

	xor	eax, eax
	mov ax, SELECTOR_TSS
	ltr	ax;

	jmp	kernel_main


; 中断和异常 -- 硬件中断,现在把hwint00里的中断处理框架拓展
; ---------------------------------
%macro  hwint_master    1
	call	save				;注释的部分包含保存现场和重入与否的分支都并入save

	;----------------------------------------
	; 关时钟中断，这是调整后新加的, 进一步禁止当前中断的重入，但是允许其他中断的重入
	; 原来是没有禁止
	in	al, INT_M_CTLMASK
	or	al, (1 << %1)						; 从原来的时钟中断通用化到任何中断
	out	INT_M_CTLMASK, al
	;----------------------------------------

	mov	al, EOI					; 注意这个是允许这次中断结束后下次时钟中断还能被响应(但在这个中断处理程序中中断是关闭的，也就是无法嵌套)
	out INT_M_CTL, al	; master 8259

	; 但此时，当前中断已经被关了， 所以后面的部分不会再被当前中断重入。
	sti	;开中断，后面是中断例程功能部分, 因为cpu响应中断的时候会自动关闭中断 
	
	; 中断处理程序调用
	push %1
	; call clock_handler, 通过函数指针数组，将原来的方法拓展到全部中断处理程序
	call	[irq_table + 4 * %1]	;  | 中断处理程序	， irq_table中断处理程序函数指针数组，存放各个中断处理程序函数的地址
	add	esp, 4		; 调用者恢复堆栈，原文是pop ecx，效果一样，因为这里得到的ecx没用

	cli

	;----------------------------------------
	; 再打开时钟中断
	; 原来是没有禁止，但是在clock_handler里判断是否的时钟中断重入，现在其实clock_handler里可以不用判断了
	in	al, INT_M_CTLMASK
	and	al, ~(1 << %1)		;  | 恢复接受当前中断
	out	INT_M_CTLMASK, al
	;----------------------------------------

	ret				;新加的，通过ret+参数的形式进行分支转移
%endmacro
; ---------------------------------

ALIGN   16
hwint00:                ; Interrupt routine for irq 0 (the clock).
	hwint_master    0			 ; 恢复原来的统一格式，但是hwint_master已经大不同了

ALIGN   16
hwint01:                ; Interrupt routine for irq 1 (keyboard)
        hwint_master    1

ALIGN   16
hwint02:                ; Interrupt routine for irq 2 (cascade!)
        hwint_master    2

ALIGN   16
hwint03:                ; Interrupt routine for irq 3 (second serial)
        hwint_master    3

ALIGN   16
hwint04:                ; Interrupt routine for irq 4 (first serial)
        hwint_master    4

ALIGN   16
hwint05:                ; Interrupt routine for irq 5 (XT winchester)
        hwint_master    5

ALIGN   16
hwint06:                ; Interrupt routine for irq 6 (floppy)
        hwint_master    6

ALIGN   16
hwint07:                ; Interrupt routine for irq 7 (printer)
        hwint_master    7

; ---------------------------------
; 从片硬件中断
%macro  hwint_slave     1
        ; push    %1
        ; call    spurious_irq
        ; add     esp, 4
        ; hlt
	call	save
	;----------------------------------------
	; 关时钟中断，这是调整后新加的, 进一步禁止当前中断的重入，但是允许其他中断的重入
	; 原来是没有禁止
	in	al, INT_S_CTLMASK
	or	al, (1 << (%1 - 8) )						; 从原来的时钟中断通用化到任何中断，从片要-8
	out	INT_S_CTLMASK, al
	;----------------------------------------

	mov	al, EOI					; 注意这个是允许这次中断结束后下次时钟中断还能被响应(但在这个中断处理程序中中断是关闭的，也就是无法嵌套)
	out INT_M_CTL, al	; master 8259
	nop
	out INT_S_CTL, al	; 从片连接的设备发生中断后主片和从片都要置EOI否则硬件后面将无法响应中断
	; 但此时，当前中断已经被关了， 所以后面的部分不会再被当前中断重入。
	sti	;开中断，后面是中断例程功能部分, 因为cpu响应中断的时候会自动关闭中断 
	
	; 中断处理程序调用
	push %1
	; call clock_handler, 通过函数指针数组，将原来的方法拓展到全部中断处理程序
	call	[irq_table + 4 * %1]	;  | 中断处理程序	， irq_table中断处理程序函数指针数组，存放各个中断处理程序函数的地址
	add	esp, 4		; 调用者恢复堆栈，原文是pop ecx，效果一样，因为这里得到的ecx没用

	cli

	;----------------------------------------
	; 再打开时钟中断
	; 原来是没有禁止，但是在clock_handler里判断是否的时钟中断重入，现在其实clock_handler里可以不用判断了
	in	al, INT_S_CTLMASK
	and	al, ~(1 << (%1 - 8) )		;  | 恢复接受当前中断
	out	INT_S_CTLMASK, al		; 相应宏改成从片的
	;----------------------------------------

	ret				;新加的，通过ret+参数的形式进行分支转移

%endmacro
; ---------------------------------

ALIGN   16
hwint08:                ; Interrupt routine for irq 8 (realtime clock).
        hwint_slave     8

ALIGN   16
hwint09:                ; Interrupt routine for irq 9 (irq 2 redirected)
        hwint_slave     9

ALIGN   16
hwint10:                ; Interrupt routine for irq 10
        hwint_slave     10

ALIGN   16
hwint11:                ; Interrupt routine for irq 11
        hwint_slave     11

ALIGN   16
hwint12:                ; Interrupt routine for irq 12
        hwint_slave     12

ALIGN   16
hwint13:                ; Interrupt routine for irq 13 (FPU exception)
        hwint_slave     13

ALIGN   16
hwint14:                ; Interrupt routine for irq 14 (AT winchester)
        hwint_slave     14

ALIGN   16
hwint15:                ; Interrupt routine for irq 15
        hwint_slave     15

; 中断和异常, 最后统一跳到后面调用exception_handler函数
divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
double_fault:
	push	8		; vector_no	= 8
	jmp	exception
copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
inval_tss:
	push	10		; vector_no	= A
	jmp	exception
segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
stack_exception:
	push	12		; vector_no	= C
	jmp	exception
general_protection:
	push	13		; vector_no	= D
	jmp	exception
page_fault:
	push	14		; vector_no	= E
	jmp	exception
copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	call	exception_handler
	; 处理完，根据C调用规则，调用放恢复堆栈
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt							; 暂时没往下写的时候就hlt



; ====================================================================================
;                                   save：进一步模块化，把前面的保存现场和刚刚写好的后面的是否重入分支统一到save
; ====================================================================================
save:
	; sub	esp, 4			; 跳过retaddr			这个已经由call自动入栈，无需手动跳过
	pushad
	push	ds
	push	es
	push	fs
	push	gs

	; 这里因为后面用到了dx来传系统调用参数，所以dx要先保护一下
	mov	esi, edx

	mov	dx, ss	;让ds和es指向与ss相同的段
	mov	ds, dx
	mov	es, dx
	mov fs, dx

	mov edx, esi

	mov		esi, esp		; eax = 进程表起始地址，用来通过进程表项来从save函数返回

	;	是否重入的判断和解决也在save里解决，注意重入不单单指时钟中断重入时钟中断，还有别的中断重入时钟中断
	; 但是只要是重入，k_reenter都会被更新。所以clock_handler里判断是否重入暂时还是有必要的，为了识别别的中断的重入。
	inc	dword	[k_reenter]
	cmp	dword	[k_reenter], 0
	jne		.1
	mov	esp, StackTop		; 非重入
	push	restart
	jmp	[esi + RETADR - P_STACKBASE]

.1:
	push	restart_reenter
	jmp	[esi + RETADR - P_STACKBASE]



; ====================================================================================
;                                 sys_call:系统调用中断处理函数
; ====================================================================================
sys_call:
	call	save


	sti
	push	esi
	
	push	dword	[p_proc_ready]			; sys_write比write多一个参数，就是调用该系统调用的进程标识
																				; 在这里就是当前进行的指针p_proc_ready

	; 前面的sys_get_ticks没有参数所以没注意这个问题
	; 系统调用虽然用了函数指针表数组的形式来统一通过一个中断函数实现调用
	; 但是各个系统调用的参数列表不一样，这个如何解决？？后面再看
	push edx
	push ecx
	push ebx
	call	[sys_call_table + eax * 4]		; 系统调用中参数最多的是4个
	add esp, 4 * 4

	pop	esi						;虽然暂时还没用到esi但是也先保护一下
	mov	[esi + EAXREG - P_STACKBASE], eax		; 这个挺有意思，就是把返回值放到进程表里对应的eax域
																							; 从而保证返回调用者进程（进程恢复）后，寄存器全部恢复进程表
																							; 里的对应值后能够得到正确的返回值，也就是EAX里装的是sys_get_ticks的返回值
	cli

	ret


; ====================================================================================
;                                   restart：做好准备，并加载一个进程
; ====================================================================================
; 现在看这一段其实和中断处理程序的后半段restart_v2和restart_reenter_v2是一样的，可以合并
restart:
	; 目标ring1代码的cs\eip\ss\esp都是从堆栈得到的，这里的堆栈先手动设置成了进程表的起始地址
	mov esp, [p_proc_ready]
	lldt	[esp+P_LDT_SEL]
	lea	eax, [esp + P_STACKTOP]
	mov	dword	[tss + TSS3_S_SP0], eax		;栈顶指针存进tss，之后能从TSS中直接得到ring0下的esp值tss.esp0

restart_reenter:
	dec		dword	[k_reenter]			;为了统一第一个进程启动和进程调度的情况添加的
	pop	gs
	pop	fs
	pop	es
	pop	ds
	popad

	add	esp, 4

	iretd