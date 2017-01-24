bootloader:EasyOS.asm Makefile
	nasm -o bootloader -l ipl.lst EasyOS.asm

img:bootloader Makefile
	dd if=bootloader of=EasyOS.img bs=512 count=1
	dd if=/dev/zero of=EasyOS.img bs=512 seek=1 count=2879

run:
	make img
	qemu-system-i386 -fda EasyOS.img
