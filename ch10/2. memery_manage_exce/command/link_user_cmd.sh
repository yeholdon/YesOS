gcc -m32 -I ../include -I ../include/sys -c -fno-builtin -fno-stack-protector -Wall -o pwd.o pwd.c
nasm -I ../include/ -f elf -o start.o start.asm
ld -m elf_i386 -Ttext 0x1000 -o pwd pwd.o start.o ../lib/yescrt.a
