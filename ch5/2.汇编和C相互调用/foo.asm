; 编译链接方法
; (ld 的‘-s’选项意为“strip all”)
;
; $ nasm -f elf foo.asm -o foo.o
; $ gcc -c bar.c -o bar.o
; $ ld -s hello.o bar.o -o foobar
; $ ./foobar
; the 2nd one
; $

; 用extern关键字来声明本文件之外的函数名
extern choose	; int choose(int a, int b);

[section .data]	; 数据在此, choose()函数的两个参数

num1st		dd	3
num2nd		dd	4

[section .text]	; 代码在此

global _start	; 我们必须导出 _start 这个入口，以便让链接器识别。
global myprint	; 导出这个函数为了让 bar.c 使用

_start:
	; 汇编的参数通过栈传递（调用C程序就必须通过栈传递),而且有趣的是这里并没有自己定义栈段
	; 遵循C Calling Convention，后面参数先入栈，并由调用者Caller清栈
	push	dword [num2nd]	; `.
	push	dword [num1st]	;  |
	call	choose		;  | choose(num1st, num2nd);
	add	esp, 8		; /	调用者清栈

	mov	ebx, 0
	mov	eax, 1		; sys_exit
	int	0x80		; 系统调用

; void myprint(char* msg, int len)
myprint:
	mov	edx, [esp + 8]	; len
	mov	ecx, [esp + 4]	; msg
	mov	ebx, 1
	mov	eax, 4		; sys_write
	int	0x80		; 系统调用
	ret
	
