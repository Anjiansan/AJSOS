#include<stdio.h>
#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *binfo=(struct BOOTINFO *)0x0FF0;
    char s[40],mcursor[256];
    int mx,my;

    init_gdtidt();
    init_pic();
    io_sti();//开中断

    init_palette();//设定调色板
    init_screen(binfo->vram,binfo->scrnx,binfo->scrny);
    mx=(binfo->scrnx-16)/2;//画面中央坐标
    my=(binfo->scrny-28-16)/2;
    init_mouse_cursor8(mcursor,COL8_008484);
    putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
    sprintf(s,"(%d,%d)",mx,my);
    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);

    io_out8(PIC0_IMR,0xF9);
    io_out8(PIC1_IMR,0xEF);

    for(;;)
    {
        io_hlt();
    }
}
