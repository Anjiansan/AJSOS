#include "bootpack.h"

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

#define KEYCMD_SENDTO_MOUSE 0xD4
#define MOUSECMD_ENABLE 0xF4

void enable_mouse(struct MOUSE_DEC *mdec)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD,KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT,MOUSECMD_ENABLE);
    // 等待0xFA的阶段
    mdec->phase=0;
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat)
{
    if(mdec->phase==0)
    {//等待鼠标的0xFA的状态
        if(dat==0xFA)
        {
            mdec->phase=1;
        }
        return 0;
    }
    if(mdec->phase==1)
    {//等待鼠标的第一字节
        if((dat & 0xC8)==0x08)
        {//第一字节检查:前四位:0-3,后四位:8-F
            mdec->buf[0]=dat;
            mdec->phase=2;
        }
        return 0;
    }
    if(mdec->phase==2)
    {//等待鼠标的第二字节
        mdec->buf[1]=dat;
        mdec->phase=3;
        return 0;
    }
    if(mdec->phase==3)
    {//等待鼠标的第三字节
        mdec->buf[2]=dat;
        mdec->phase=1;

        mdec->btn=mdec->buf[0] & 0x07;
        mdec->x=mdec->buf[1];
        mdec->y=mdec->buf[2];
        if((mdec->buf[0] & 0x10)!=0)
        {
            mdec->x |= 0xFFFFFF00;
        }
        if((mdec->buf[0] & 0x20)!=0)
        {
            mdec->y |= 0xFFFFFF00;
        }
        mdec->y=-mdec->y;//鼠标的y方向与画面符号相反

        return 1;
    }
    return -1;
}
