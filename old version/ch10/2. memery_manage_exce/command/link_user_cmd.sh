gcc -m32 -O0 -I ../include -I ../include/sys -c -fno-builtin -fno-stack-protector -Wall -o echo.o echo.c
nasm -I ../include/ -f elf -o start.o start.asm
ld -m elf_i386 -Ttext 0x1000 -o echo echo.o start.o ../lib/yescrt.a
