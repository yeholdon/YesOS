# Makefile for boot, at root directory

# programs, flags, etc
ASM  = nasm
ASMFLAGS = -I boot/include/

# This Program, don't TARGET
YESBOOT = boot/boot.bin boot/loader.bin

# All Phony Targets
.PHONY : everything clean all

# Default starting position
everything : $(YESBOOT)

clean :
	rm -f $(YESBOOT)

# clean and remake
all : clean everything

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc boot/include/fat12hdr.inc boot/include/pm.inc boot/include/lib.inc
	$(ASM) $(ASMFLAGS) -o $@ $<
