;*****************************************************
 ; syscall.asm: 新的其他类型的中断，准确的说是用于系统调用的软件中断
 ; 程序功能：定义系统调用中断处理函数
 ; 修改日期：2019.12.5
 ;/

%include    "sconst.inc"

_NR_get_ticks   equ 0           ;等价于irq
_NR_write   equ 1

INT_VECTOR_SYS_CALL equ 0x90            ;系统调用的中断向量号。不和别的重复即可

global  get_ticks       ;导出系统调用的符号
global  write

bits    32
[section    .text]

; ====================================================================
;                              get_ticks():获取时钟中断次数
; ====================================================================
get_ticks:
    mov eax, _NR_get_ticks          ; 系统调用标号通过eax寄存器传入
    int    INT_VECTOR_SYS_CALL      ;软中断，触发相应的中断处理函数sys_call
    ret


; ====================================================================================
;                          void write(char* buf, int len):
; ====================================================================================
write:          ; 这里只管传参数即可，实现主体用C语言
    mov     eax, _NR_write
    mov     ebx, [esp + 4]
    mov     ecx, [esp + 8]
    int     INT_VECTOR_SYS_CALL
    ret