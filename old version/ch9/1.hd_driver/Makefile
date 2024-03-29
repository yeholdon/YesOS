################################
# Makefile for whole project : Ye's OS  #
################################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x30400

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf
CFLAGS		= -m32 -I include/ -c -fno-builtin -fno-stack-protector
LDFLAGS		= -m elf_i386 -s -Ttext $(ENTRYPOINT)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
YESBOOT	= boot/boot.bin boot/loader.bin
YESKERNEL	= kernel.bin
OBJS		= kernel/kernel.o kernel/syscall.o  kernel/keyboard.o kernel/start.o kernel/init8259.o  \
						kernel/proc.o kernel/global.o kernel/protect.o  kernel/main.o kernel/clock.o \
						kernel/tty.o kernel/console.o kernel/systask.o \
						kernel/printf.o kernel/vsprintf.o kernel/hd.o \
						lib/klib.o lib/kliba.o lib/string.o lib/misc.o \
						fs/main.o
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

kernel/kernel.o : kernel/kernel.asm	include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/syscall.o : kernel/syscall.asm	include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o: kernel/start.c include/type.h include/const.h  \
											include/protect.h include/proto.h include/string.h  include/proc.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/init8259.o: kernel/init8259.c  include/type.h  include/const.h \
													include/protect.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<


#### 设置中断和IDT时添加的
kernel/global.o : kernel/global.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o : kernel/protect.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c
	$(CC) $(CFLAGS) -o $@ $<

lib/klib.o : lib/klib.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc.o : lib/misc.c
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
kernel/printf.o : kernel/printf.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/vsprintf.o : kernel/vsprintf.c
	$(CC) $(CFLAGS) -o $@ $<

#### 文件系统
fs/main.o: fs/main.c
	$(CC) $(CFLAGS) -o $@ $<
