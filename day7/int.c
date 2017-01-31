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

#define PORT_KEYDAT 0x0060

struct FIFO8 keyfifo;

void inthandler21(int *esp)
{
    unsigned char data;
    io_out8(PIC0_OCW2,0x61);//通知PIC"IRQ-01已经受理完毕"
    data=io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo,data);
    return;
}

struct FIFO8 mousefifo;

void inthandler2c(int *esp)
{
    unsigned char data;
    io_out8(PIC1_OCW2,0x64);//通知PIC"IRQ-12已经受理完毕"
    io_out8(PIC0_OCW2,0x62);//通知PIC"IRQ-02已经受理完毕"
    data=io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo,data);
    return;
}

void inthandler27(int *esp)
{
    io_out8(PIC0_OCW2,0x67);
    return;
}
