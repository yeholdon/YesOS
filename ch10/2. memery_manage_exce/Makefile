################################
# Makefile for whole project : Ye's OS  #
################################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x1000

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM		= objdump
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -I include/sys/ -f elf
# CFLAGS		= -m32 -I include/ -c -fno-builtin -fno-stack-protector
CFLAGS		= -m32 -I include/ -I include/sys/ -c -fno-builtin -fno-stack-protector
LDFLAGS		= -m elf_i386 -s -Ttext $(ENTRYPOINT) -Map krnl.map
DASMFLAGS	= -D

# This Program
YESBOOT	= boot/boot.bin boot/loader.bin
YESKERNEL	= kernel.bin
OBJS		= kernel/kernel.o kernel/keyboard.o kernel/start.o kernel/init8259.o  \
						kernel/proc.o kernel/global.o kernel/protect.o  kernel/main.o kernel/clock.o \
						kernel/tty.o kernel/console.o kernel/systask.o kernel/hd.o \
						lib/klib.o lib/kliba.o lib/string.o lib/misc.o lib/open.o lib/close.o \
						lib/read.o lib/write.o lib/printf.o lib/vsprintf.o lib/syslog.o lib/syscall.o  \
						lib/getpid.o lib/unlink.o lib/fork.o lib/exit.o lib/wait.o \
						fs/main.o fs/open.o fs/misc.o fs/read_write.o fs/disklog.o fs/link.o \
						mm/main.o mm/fork.o mm/exit.o mm/wait.o
DASMOUTPUT	= kernel.bin.asm

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg

# Default starting position
everything : $(YESBOOT) $(YESKERNEL)

all : realclean everything

final : all clean

image : final buildimg

clean :
	rm -f $(OBJS)

# clean .o files and target files
realclean :
	rm -f $(OBJS) $(YESBOOT) $(YESKERNEL)

disasm :
	$(DASM) $(DASMFLAGS) $(YESKERNEL) > $(DASMOUTPUT)

# We assume that "a.img" exists in current folder
buildimg :
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc \
			boot/include/fat12hdr.inc boot/include/pm.inc boot/include/lib.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(YESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(YESKERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm	
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o: kernel/start.c 
	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.c 
	$(CC) $(CFLAGS) -o $@ $<

kernel/init8259.o: kernel/init8259.c  
	$(CC) $(CFLAGS) -o $@ $<


#### 设置中断和IDT时添加的
kernel/global.o : kernel/global.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o : kernel/protect.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

lib/syscall.o : lib/syscall.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/klib.o : lib/klib.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc.o : lib/misc.c
	$(CC) $(CFLAGS) -o $@ $<

lib/open.o : lib/open.c
	$(CC) $(CFLAGS) -o $@ $<

lib/close.o : lib/close.c
	$(CC) $(CFLAGS) -o $@ $<

lib/read.o : lib/read.c
	$(CC) $(CFLAGS) -o $@ $<

lib/write.o : lib/write.c
	$(CC) $(CFLAGS) -o $@ $<

lib/syslog.o : lib/syslog.c
	$(CC) $(CFLAGS) -o $@ $<

lib/getpid.o : lib/getpid.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fork.o : lib/fork.c
	$(CC) $(CFLAGS) -o $@ $<

lib/exit.o : lib/exit.c
	$(CC) $(CFLAGS) -o $@ $<

lib/wait.o : lib/wait.c
	$(CC) $(CFLAGS) -o $@ $<

lib/kliba.o : lib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<



#### 系统调用和进程
kernel/proc.o : kernel/proc.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/systask.o : kernel/systask.c
	$(CC) $(CFLAGS) -o $@ $<

#### IO系统
kernel/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/tty.o : kernel/tty.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/console.o : kernel/console.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/hd.o: kernel/hd.c
	$(CC) $(CFLAGS) -o $@ $<

#### C库函数
lib/printf.o : lib/printf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/vsprintf.o : lib/vsprintf.c
	$(CC) $(CFLAGS) -o $@ $<

#### 文件系统fs
fs/main.o: fs/main.c
	$(CC) $(CFLAGS) -o $@ $<

fs/open.o:fs/open.c
	$(CC) $(CFLAGS) -o $@ $<

fs/misc.o:fs/misc.c
	$(CC) $(CFLAGS) -o $@ $<

fs/read_write.o:fs/read_write.c
	$(CC) $(CFLAGS) -o $@ $<

fs/disklog.o:fs/disklog.c
	$(CC) $(CFLAGS) -o $@ $<

#### 内存管理mm
mm/main.o : mm/main.c
	$(CC) $(CFLAGS) -o $@ $<

mm/fork.o : mm/fork.c
	$(CC) $(CFLAGS) -o $@ $<

mm/exit.o : mm/exit.c
	$(CC) $(CFLAGS) -o $@ $<

mm/wait.o : mm/wait.c
	$(CC) $(CFLAGS) -o $@ $<