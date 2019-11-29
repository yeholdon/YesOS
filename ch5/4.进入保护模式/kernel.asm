; $ nasm -f eflf kernel.asm -o kernel.o
; $ ld -m elf_i386 -s kernel.o -o kernel.bin    #64位Linux链接要加-m

[section .text] ; 代码段在elf里通常写成.text

global  _start:

_start:
    ;假设此时gs已经指向显存了
    mov ah, 0Fh     ; 0000: 黑底    1111: 白字
	mov	al, 'K'
	mov	[gs:((80 * 1 + 39) * 2)], ax	; 屏幕第 1 行, 第 39 列。

	jmp	$				; 到此停住