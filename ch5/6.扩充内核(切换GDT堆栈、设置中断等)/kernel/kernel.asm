
; 内核段选择子CS=Core Segment，TI=RPL=0
SELECTOR_KERNEL_CS  equ 8

; 导入函数调用别人的，c语言函数
extern  cstart
; 导入全局变量
extern  gdt_ptr

[SECTION .bss]      ; Block Started by Symbol
StackSpace	resb	2*1024
StackTop:

[SECTION .text]		; 代码
global _start	;导出_start让链接器知道入口

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

	;更新gdt的地址
	sgdt	[gdt_ptr]				;保存gdt所在的段地址和偏移地址内存	
	call	cstart						  ;cstart()函数改变gdt_ptr使其指向内核里的gdt
	lgdt	[gdt_ptr]			

	jmp SELECTOR_KERNEL_CS:csinit	;“这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.
																			;更新了高速缓存中的内容

csinit:
	push	0		;清空标准寄存器
	popfd			; Pop top of stack into EFLAGS

	hlt					;切换工作完成