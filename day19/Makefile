OBJS_BOOTPACK=bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj \
			  fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj mtask.obj \
			  window.obj console.obj file.obj

TOOLS_PATH	:= /home/anjiansan/Document/OS/AJSOS/z_tools/
INCPATH		:= $(TOOLS_PATH)haribote/

CC1 		:= wine $(TOOLS_PATH)cc1.exe -I $(INCPATH) -Os -Wall -quiet
GAS2NASK 	:= wine $(TOOLS_PATH)gas2nask.exe -a
NASK 		:= wine $(TOOLS_PATH)nask.exe
OBJ2BIM 	:= wine $(TOOLS_PATH)obj2bim.exe
RULEFILE 	:= $(TOOLS_PATH)haribote/haribote.rul
BIM2HRB 	:= wine $(TOOLS_PATH)bim2hrb.exe
MAKEFONT	:= wine $(TOOLS_PATH)makefont.exe
BIN2OBJ		:= wine $(TOOLS_PATH)bin2obj.exe

bootloader:ipl.nas Makefile
	$(NASK) ipl.nas bootloader ipl.lst

asmhead.bin:asmhead.nas
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

hankaku.bin:hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj:hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim:$(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
	$(OBJS_BOOTPACK)
#3MB+64KB=3136KB

bootpack.hrb:bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

sys:asmhead.bin bootpack.hrb
	cat asmhead.bin bootpack.hrb > AJSOS.sys

img:bootloader Makefile
	dd if=bootloader of=AJSOS.img bs=512 count=1
	dd if=/dev/zero of=AJSOS.img bs=512 seek=1 count=2879

# 一般规则
%.gas:%.c Makefile
	$(CC1) -o $*.gas $*.c

%.nas:%.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj:%.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

copy:
	mkdir /tmp/floppy
	mount -o loop AJSOS.img /tmp/floppy
	sleep 1
	cp AJSOS.sys /tmp/floppy
	cp ipl.nas /tmp/floppy
	cp Makefile /tmp/floppy
	cp hlt.ajs /tmp/floppy
	sleep 1
	umount /tmp/floppy
	rmdir /tmp/floppy

run:
	make img
	make sys
	make copy
	qemu-system-i386 -fda AJSOS.img

clean:
	rm -f *.bin
	rm -f *.lst
	rm -f *.gas
	rm -f *.obj
	rm -f bootloader
	rm -f bootpack.nas
	rm -f bootpack.nask
	rm -f bootpack.map
	rm -f bootpack.bim
	rm -f bootpack.hrb
	rm -f AJSOS.sys

src_only:
	make clean
	rm -f AJSOS.img
