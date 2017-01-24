bootloader:EasyOS.asm Makefile
	nasm -o bootloader -l ipl.lst EasyOS.asm

img:bootloader Makefile
	dd if=/dev/zero of=EasyOS.img bs=512 count=2880
	dd if=bootloader of=EasyOS.img bs=512 count=1

run:
	make img
	qemu-system-i386 EasyOS.img
