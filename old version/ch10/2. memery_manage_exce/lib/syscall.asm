;*****************************************************
 ; syscall.asm: 新的其他类型的中断，准确的说是用于系统调用的软件中断
 ; 程序功能：定义系统调用中断处理函数
 ; 修改日期：2019.12.5
 ;/

%include    "sconst.inc"

_NR_get_ticks   equ 0           ;等价于irq
_NR_write   equ 1
_NR_sendrec equ 2
_NR_printx  equ 3

INT_VECTOR_SYS_CALL equ 0x90            ;系统调用的中断向量号。不和别的重复即可

global  get_ticks_syscall_version       ;导出系统调用的符号
global  write_syscall_version
global sendrec
global printx

bits    32
[section    .text]

; ====================================================================
;                              get_ticks():获取时钟中断次数
; ====================================================================
get_ticks_syscall_version:
    mov eax, _NR_get_ticks          ; 系统调用标号通过eax寄存器传入
    int    INT_VECTOR_SYS_CALL      ;软中断，触发相应的中断处理函数sys_call
    ret


; ====================================================================================
;                          void write(char* buf, int len):
; ====================================================================================
write_syscall_version:          ; 这里只管传参数即可，实现主体用C语言
    push edx
    push ecx
    mov     eax, _NR_write
    mov     edx, [esp + 8 + 4]
    mov     ecx, [esp + 8 + 8]
    int     INT_VECTOR_SYS_CALL
    pop ecx
    pop edx
    ret

; ====================================================================================
;                  sendrec(int function, int src_dest, MESSAGE* msg);
; ====================================================================================
; Never call sendrec() directly, call send_recv() instead.
sendrec:
    push ebx
    push ecx
    push edx

    mov eax, _NR_sendrec
    mov ebx, [esp + 12 + 4]
    mov ecx, [esp + 12 +  8]
    mov edx, [esp + 12 + 12]
    int INT_VECTOR_SYS_CALL

	pop	edx
	pop	ecx
	pop	ebx

    ret


; ====================================================================================
;                          void printx(char* s);
; ====================================================================================
printx:
    push edx
	mov	eax, _NR_printx
	mov	edx, [esp + 4 + 4]
	int	INT_VECTOR_SYS_CALL
    pop edx
	ret
