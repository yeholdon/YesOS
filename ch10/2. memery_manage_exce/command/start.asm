;; start.asm
; 作用：1.为main()函数准备参数   2. 调用main()   3. 将main函数的返回值通过exit()传递给父进程

extern	main
extern	exit

bits 32

[section .text]

global _start

_start:
	push	eax
	push	ecx
	call	main
	;; need not clean up the stack here

	push	eax
	call	exit

	hlt	; should never arrive here