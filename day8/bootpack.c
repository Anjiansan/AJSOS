#include<stdio.h>
#include "bootpack.h"

struct MOUSE_DEC
{
    unsigned char buf[3],phase;
    int x,y,btn;
};

extern struct FIFO8 keyfifo,mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat);

void HariMain(void)
{
    struct BOOTINFO *binfo=(struct BOOTINFO *)0x0FF0;
    char s[40],mcursor[256],keybuf[32],mousebuf[128];
    int mx,my,i;
    struct MOUSE_DEC mdec;

    init_gdtidt();
    init_pic();
    io_sti();//开中断

    fifo8_init(&keyfifo,32,keybuf);
    fifo8_init(&mousefifo,128,mousebuf);
    io_out8(PIC0_IMR,0xF9);
    io_out8(PIC1_IMR,0xEF);

    init_keyboard();

    init_palette();//设定调色板
    init_screen(binfo->vram,binfo->scrnx,binfo->scrny);
    mx=(binfo->scrnx-16)/2;//画面中央坐标
    my=(binfo->scrny-28-16)/2;
    init_mouse_cursor8(mcursor,COL8_008484);
    putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
    sprintf(s,"(%d,%d)",mx,my);
    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);

    enable_mouse(&mdec);

    for(;;)
    {
        io_cli();
        if(fifo8_status(&keyfifo)+fifo8_status(&mousefifo)==0)
        {
            io_stihlt();
        }
        else
        {
            if(fifo8_status(&keyfifo)!=0)
            {
                i=fifo8_get(&keyfifo);
                io_sti();

                sprintf(s,"%02X",i);
                boxfill8(binfo->vram,binfo->scrnx,COL8_008484,0,16,15,31);
                putfont8_asc(binfo->vram,binfo->scrnx,0,16,COL8_FFFFFF,s);
            }
            else if(fifo8_status(&mousefifo)!=0)
            {
                i=fifo8_get(&mousefifo);
                io_sti();
                if(mouse_decode(&mdec,i)!=0)
                {
                // 鼠标的3个字节都齐了,显示
                   sprintf(s,"[lcr %4d %4d]",mdec.x,mdec.y);
                   if((mdec.btn & 0x01)!=0)
                   {
                       s[1]='L';
                   }
                   if((mdec.btn & 0x02)!=0)
                   {
                       s[3]='R';
                   }
                   if((mdec.btn & 0x04)!=0)
                   {
                       s[2]='C';
                   }
                   boxfill8(binfo->vram,binfo->scrnx,COL8_008484,32,16,32+15*8-1,31);
                   putfont8_asc(binfo->vram,binfo->scrnx,32,16,COL8_FFFFFF,s);

                //    鼠标指针的移动
                    boxfill8(binfo->vram,binfo->scrnx,COL8_008484,mx,my,mx+15,my+15);//隐藏鼠标
                    mx+=mdec.x;
                    my+=mdec.y;
                    if(mx<0)
                    {
                        mx=0;
                    }
                    if(my<0)
                    {
                        my=0;
                    }
                    if(mx>binfo->scrnx-16)
                    {
                        mx=binfo->scrnx-16;
                    }
                    if(my>binfo->scrny-16)
                    {
                        my=binfo->scrny-16;
                    }
                    sprintf(s,"(%3d,%3d)",mx,my);
                    boxfill8(binfo->vram,binfo->scrnx,COL8_008484,0,0,79,15);//隐藏坐标
                    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);//显示坐标
                    putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);//描画鼠标
                }
            }
        }
    }
}

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

void wait_KBC_sendready(void)
{
    for(;;)
    {
        if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY)==0)
        {
            break;
        }
    }
    return;
}

void init_keyboard(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD,KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT,KBC_MODE);
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
