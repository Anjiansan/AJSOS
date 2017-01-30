#include "bootpack.h"

//PIC初始化
void init_pic(void)
{
    io_out8(PIC0_IMR,0xFF);//禁止所有中断
    io_out8(PIC1_IMR,0xFF);//禁止所有中断

    io_out8(PIC0_ICW1,0x11);//边沿触发模式
    io_out8(PIC0_ICW2,0x20);//IRQ0-7由INT20-27接受
    io_out8(PIC0_ICW3,1 << 2);//PIC1由IRQ2接受
    io_out8(PIC0_ICW4,0x01);//无缓冲模式

    io_out8(PIC1_ICW1,0x11);//边沿触发模式
    io_out8(PIC1_ICW2,0x28);//IRQ8-15由INT28-2F接受
    io_out8(PIC1_ICW3,2);//PIC1由IRQ2接受
    io_out8(PIC1_ICW4,0x01);//无缓冲模式

    io_out8(PIC0_IMR,0xFB);//PIC1以外全部禁止
    io_out8(PIC1_IMR,0xFF);//禁止所有中断

    return;
}

void inthandler21(int *esp)
{
    struct BOOTINFO *binfo=(struct BOOTINFO *)ADR_BOOTINFO;
    boxfill8(binfo->vram,binfo->scrnx,COL8_000000,0,0,32*8-1,15);
    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,"INT 21 (IRQ-1) : PS/2 keyboard");
    for(;;)
    {
        io_hlt();
    }
}

void inthandler2c(int *esp)
{
    struct BOOTINFO *binfo=(struct BOOTINFO *)ADR_BOOTINFO;
    boxfill8(binfo->vram,binfo->scrnx,COL8_000000,0,0,32*8-1,15);
    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,"INT 2C (IRQ-12) : PS/2 mouse");
    for(;;)
    {
        io_hlt();
    }
}

void inthandler27(int *esp)
{
    io_out8(PIC0_OCW2,0x67);
    return;
}
