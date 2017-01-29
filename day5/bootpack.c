#include<stdio.h>

#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

struct BOOTINFO
{
    char cyls,leds,vmode,reserve;
    short scrnx,scrny;
    char *vram;
};

struct SEGMENT_DESCRIPTOR
{
    short limit_low,base_low;
    char base_mid,access_right;
    char limit_high,base_high;
};

struct GATE_DESCRIPTOR
{
    short offset_low,selector;
    char dw_count,access_right;
    short offset_high;
};

// 告诉C编译器,有一个函数在别的文件里
void io_hlt(void);
void io_cli(void);
void io_out8(int port,int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start,int end,unsigned char *rgb);
void boxfill8(unsigned char *vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1);
void init_screen(char *vram,int xsize,int ysize);
void putfont8(char *vram,int xsize,int x,int y,char c,char *font);
void putfont8_asc(char *vram,int xsize,int x,int y,char c,unsigned char *s);
void init_mouse_cursor8(char *mouse,char bc);
void putblock8_8(char *vram,int vxsize,int pxsize,int pysize,int px0,int py0,char *buf,int bxsize);


void init_gdtidt(void);
void set_segmdsc(struct SEGMENT_DESCRIPTOR *sd,unsigned int limit,int base,int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd,int offset,int selector,int ar);
void load_gdtr(int limit,int addr);
void load_idtr(int limit,int addr);

void HariMain(void)
{
    struct BOOTINFO *binfo=(struct BOOTINFO *)0x0FF0;
    char s[40],mcursor[256];
    int mx,my;

    init_palette();//设定调色板
    init_screen(binfo->vram,binfo->scrnx,binfo->scrny);
    // putfont8_asc(binfo->vram,binfo->scrnx,8,8,COL8_FFFFFF,"ABC 123");
    // putfont8_asc(binfo->vram,binfo->scrnx,31,31,COL8_000000,"AJSOS");
    // putfont8_asc(binfo->vram,binfo->scrnx,30,30,COL8_FFFFFF,"AJSOS");
    // sprintf(s,"scrnx=%d",binfo->scrnx);
    // putfont8_asc(binfo->vram,binfo->scrnx,16,64,COL8_FFFFFF,s);
    mx=(binfo->scrnx-16)/2;//画面中央坐标
    my=(binfo->scrny-28-16)/2;
    init_mouse_cursor8(mcursor,COL8_008484);
    putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
    sprintf(s,"(%d,%d)",mx,my);
    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);

    for(;;)
    {
        io_hlt();
    }
}

void init_palette(void)
{
    static unsigned char table_rgb[16*3]=
    {
        0x00,0x00,0x00,//0:黑
        0xFF,0x00,0x00,//1:亮红
        0x00,0xFF,0x00,//2:亮绿
        0xFF,0xFF,0x00,//3:亮黄
        0x00,0x00,0xFF,//4:亮蓝
        0xFF,0x00,0xFF,//5:亮紫
        0xFF,0xFF,0xFF,//6:浅亮蓝
        0xFF,0xFF,0xFF,//7:白
        0xC6,0xC6,0xC6,//8:亮灰
        0x84,0x00,0x00,//9:暗红
        0x00,0x84,0x00,//10:暗绿
        0x84,0x84,0x00,//11:暗黄
        0x00,0x00,0x84,//12:暗青
        0x84,0x00,0x84,//13:暗紫
        0x00,0x84,0x84,//14:浅暗蓝
        0x84,0x84,0x84//15:暗灰
    };
    set_palette(0,15,table_rgb);
    return;
}

void set_palette(int start,int end,unsigned char *rgb)
{
    int i,eflags;
    eflags=io_load_eflags();//记录中断许可的值
    io_cli();//将中断许可标志置为0禁止中断
    io_out8(0x03C8,start);
    for (i = 0; i <= end ; i++)
    {
        io_out8(0x03C9,rgb[0]/4);
        io_out8(0x03C9,rgb[1]/4);
        io_out8(0x03C9,rgb[2]/4);
        rgb+=3;
    }
    io_store_eflags(eflags);//复原终端许可标志
    return;
}

void boxfill8(unsigned char *vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1)
{
    int x,y;
    for(y=y0;y<=y1;y++)
        for(x=x0;x<=x1;x++)
        {
            vram[y*xsize+x]=c;
        }
    return;
}

void init_screen(char *vram,int xsize,int ysize)
{
    boxfill8(vram,xsize,COL8_008484,0,0,xsize-1,ysize-29);
    boxfill8(vram,xsize,COL8_C6C6C6,0,ysize-28,xsize-1,ysize-28);
    boxfill8(vram,xsize,COL8_FFFFFF,0,ysize-27,xsize-1,ysize-27);
    boxfill8(vram,xsize,COL8_C6C6C6,0,ysize-26,xsize-1,ysize-1);

    boxfill8(vram,xsize,COL8_FFFFFF,3,ysize-24,59,ysize-24);
    boxfill8(vram,xsize,COL8_FFFFFF,2,ysize-24,2,ysize-4);
    boxfill8(vram,xsize,COL8_848484,3,ysize-4,59,ysize-4);
    boxfill8(vram,xsize,COL8_848484,59,ysize-23,59,ysize-5);
    boxfill8(vram,xsize,COL8_000000,2,ysize-3,59,ysize-3);
    boxfill8(vram,xsize,COL8_000000,60,ysize-24,60,ysize-3);

    boxfill8(vram,xsize,COL8_848484,xsize-47,ysize-24,xsize-4,ysize-24);
    boxfill8(vram,xsize,COL8_848484,xsize-47,ysize-23,xsize-47,ysize-4);
    boxfill8(vram,xsize,COL8_FFFFFF,xsize-47,ysize-3,xsize-4,ysize-3);
    boxfill8(vram,xsize,COL8_FFFFFF,xsize-3,ysize-24,xsize-3,ysize-3);

    return;
}

void putfont8(char *vram,int xsize,int x,int y,char c,char *font)
{
    int i;
    char *p,d;//d:data
    for(i=0;i<16;i++)
    {
        p=vram+(y+i)*xsize+x;
        d=font[i];
        if((d & 0x80)!=0){p[0]=c;}
        if((d & 0x40)!=0){p[1]=c;}
        if((d & 0x20)!=0){p[2]=c;}
        if((d & 0x10)!=0){p[3]=c;}
        if((d & 0x08)!=0){p[4]=c;}
        if((d & 0x04)!=0){p[5]=c;}
        if((d & 0x02)!=0){p[6]=c;}
        if((d & 0x01)!=0){p[7]=c;}
    }
    return;
}

void putfont8_asc(char *vram,int xsize,int x,int y,char c,unsigned char *s)
{
    extern char hankaku[4096];
    for(;*s!=0x00;s++)
    {
        putfont8(vram,xsize,x,y,c,hankaku+*s*16);
        x+=8;
    }
    return;
}

void init_mouse_cursor8(char *mouse,char bc)
{
    static char cursor[16][16]={
        "**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
    };
    int x,y;

    for(y=0;y<16;y++)
        for(x=0;x<16;x++)
        {
            if(cursor[y][x]=='*')
            {
                mouse[y*16+x]=COL8_000000;
            }
            if(cursor[y][x]=='O')
            {
                mouse[y*16+x]=COL8_FFFFFF;
            }
            if(cursor[y][x]=='.')
            {
                mouse[y*16+x]=bc;
            }
        }

    return;
}

void putblock8_8(char *vram,int vxsize,int pxsize,int pysize,int px0,int py0,char *buf,int bxsize)
{
    int x,y;
    for(y=0;y<pysize;y++)
        for(x=0;x<pxsize;x++)
        {
            vram[(py0+y)*vxsize+(px0+x)]=buf[y*bxsize+x];
        }

    return;
}


void init_gdtidt(void)
{
    struct SEGMENT_DESCRIPTOR *gdt=(struct SEGMENT_DESCRIPTOR *)0x00270000;
    struct GATE_DESCRIPTOR *idt=(struct GATE_DESCRIPTOR *)0x0026F800;
    int i;

    // GDT初始化
    for(i=0;i<8192;i++)
    {
        set_segmdsc(gdt+i,0,0,0);
    }
    set_segmdsc(gdt+1,0xFFFFFFFF,0x00000000,0x4092);
    set_segmdsc(gdt+2,0x0007FFFF,0x00280000,0x409A);
    load_gdtr(0xFFFF,0x00270000);

    // IDT初始化
    for(i=0;i<256;i++)
    {
        set_gatedesc(idt+i,0,0,0);
    }
    load_idtr(0x7FF,0x0026F800);

    return;
}

void set_segmdsc(struct SEGMENT_DESCRIPTOR *sd,unsigned int limit,int base,int ar)
{
    if(limit>0xFFFFF)
    {
        ar |= 0x8000;//G_bit=1
        limit /= 0x1000;
    }
    sd->limit_low=limit & 0xFFFF;
    sd->base_low=base & 0xFFFF;
    sd->base_mid=(base >> 16) & 0xFF;
    sd->access_right=ar & 0xFF;
    sd->limit_high=((limit >> 16) & 0x0F) | ((ar >> 8) & 0xF0);
    sd->base_high=(base >> 24) & 0xFF;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd,int offset,int selector,int ar)
{
    gd->offset_low=offset & 0xFFFF;
    gd->selector=selector;
    gd->dw_count=(ar >> 8) & 0xFF;
    gd->access_right=ar & 0xFF;
    gd->offset_high=(offset >> 16) & 0xFFFF;
    return;
}
