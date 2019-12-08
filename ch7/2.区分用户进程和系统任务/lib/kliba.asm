; kliba.asm: kernel library asm
; 程序说明：kernel的哭函数文件
; 修改日期：2019.11.29

;[SECTION .data]
;disp_pos    dd  0                                        ; 显示的起始玩位置
;现在这个当前显示位置的全局变量不再在这里汇编里定义，而是统一在global.h中定义
;这里只要导入一下这个全局变量即可

%include "sconst.inc"

extern disp_pos
[SECTION .text]

; 导出函数
global  disp_str
global	out_byte
global	in_byte
global	disp_color_str
global	disable_irq
global	enable_irq
global	enable_int
global	disable_int
; ========================================================================
;                  void disp_str(char * info);
; ========================================================================
disp_str:
    push    ebp
    push    ebx
	mov	ebp, esp        ;同样，用ebp来访问用堆栈传入的参数

	mov	esi, [ebp + 12]	; pszInfo,注意这里多入栈了一个参数后也得跟着变
	mov	edi, [disp_pos]
	mov	ah, 0Fh             ;每个字符的显示属性

.1:                                     ; 
	lodsb                           ;把si指向的存储单元读入al
	test	al, al
	jz	.2                                      ; 读完了
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3              ;不是就直接跳向显示
	push	eax     ;是回车，则先执行回车换行
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax             ; 得到行号，行号加1
	mov	bl, 160
	mul	bl              ;得到下一行行首的位置
	mov	edi, eax
	pop	eax
	jmp	.1

.3:                                 ;显示一个字符
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:                                 ;更新显示完后的当前光标位置
	mov	[disp_pos], edi
    pop ebx
	pop	ebp
	ret

; ========================================================================
;                  void disp_color_str(char * info, int color);
;					其实也就比disp_str多传入一个color的属性参数
; ========================================================================
disp_color_str:
	push	ebp
	push	ebx
	mov	ebp, esp

	mov	esi, [ebp + 12]	; pszInfo
	mov	edi, [disp_pos]
	mov	ah, [ebp + 16]	; color
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[disp_pos], edi
	pop	ebx
	pop	ebp
	ret


; ========================================================================
;                  void out_byte(u16 port, u8 value);
; ========================================================================
out_byte:
	mov	edx, [esp + 4]		; port				;也可以直接用esp来索引获取参数
	mov	al, [esp + 4 + 4]	; value				; 参数从右往左入栈
	out	dx, al
	nop	; 一点延迟
	nop
	ret

; ========================================================================
;                  u8 in_byte(u16 port);
; ========================================================================
in_byte:
	mov	edx, [esp + 4]		; port
	xor	eax, eax
	in	al, dx
	nop	; 一点延迟
	nop
	ret



; ========================================================================
;                  void disable_irq(int irq);
; ========================================================================
; Disable an interrupt request line by setting an 8259 bit.
; Equivalent code:
;	if(irq < 8)
;		out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) | (1 << irq));
;	else
;		out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) | (1 << irq));
disable_irq:
        mov     ecx, [esp + 4]          ; irq
        pushf										; push flags
        cli
        mov     ah, 1
        rol     ah, cl                  ; ah = (1 << (irq % 8))
        cmp     cl, 8
        jae     disable_8               ; disable irq >= 8 at the slave 8259
disable_0:
        in      al, INT_M_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah							; 1 = disable
        out     INT_M_CTLMASK, al       ; set bit at master 8259
        popf
        mov     eax, 1                  ; disabled by this function, 
        ret
disable_8:
        in      al, INT_S_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_S_CTLMASK, al       ; set bit at slave 8259
        popf
        mov     eax, 1                  ; disabled by this function
        ret
dis_already:
        popf
        xor     eax, eax                ; already disabled
		; eax = return value
        ret

; ========================================================================
;                  void enable_irq(int irq);
; ========================================================================
; Enable an interrupt request line by clearing an 8259 bit.
; Equivalent code:
;       if(irq < 8)
;               out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) & ~(1 << irq));
;       else
;               out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) & ~(1 << irq));
;
enable_irq:
        mov     ecx, [esp + 4]          ; irq
        pushf
        cli
        mov     ah, ~1
        rol     ah, cl                  ; ah = ~(1 << (irq % 8))
        cmp     cl, 8
        jae     enable_8                ; enable irq >= 8 at the slave 8259
enable_0:
        in      al, INT_M_CTLMASK
        and     al, ah
        out     INT_M_CTLMASK, al       ; clear bit at master 8259
        popf
        ret
enable_8:
        in      al, INT_S_CTLMASK
        and     al, ah
        out     INT_S_CTLMASK, al       ; clear bit at slave 8259
        popf
        ret


; ========================================================================
;                  void enable_int() : 打开中断
; ========================================================================
enable_int:
	sti
	ret

; ========================================================================
;                  void disable_int() : 关闭中断
; ========================================================================
disable_int:
	sti
	ret