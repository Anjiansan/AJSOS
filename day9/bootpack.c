#include<stdio.h>
#include "bootpack.h"

#define MEMMAN_FREES 4090   //大约是32KB
#define MEMMAN_ADDR 0x003C0000

struct FREEINFO             //可用信息
{
    unsigned int addr,size;
};

struct MEMMAN          //内存管理
{
    int frees,maxfrees,lostsize,losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start,unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man,unsigned int size);
int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size);

void HariMain(void)
{
    struct BOOTINFO *binfo=(struct BOOTINFO *)0x0FF0;
    char s[40],mcursor[256],keybuf[32],mousebuf[128];
    int mx,my,i;
    unsigned int memtotal;
    struct MOUSE_DEC mdec;
    struct MEMMAN *memman=(struct MEMMAN *)MEMMAN_ADDR;

    init_gdtidt();
    init_pic();
    io_sti();//开中断

    fifo8_init(&keyfifo,32,keybuf);
    fifo8_init(&mousefifo,128,mousebuf);
    io_out8(PIC0_IMR,0xF9);
    io_out8(PIC1_IMR,0xEF);

    init_keyboard();
    enable_mouse(&mdec);
    memtotal=memtest(0x00400000,0xBFFFFFFF);
    memman_init(memman);
    memman_free(memman,0x00001000,0x0009E000);
    memman_free(memman,0x00400000,memtotal-0x00400000);

    init_palette();//设定调色板
    init_screen(binfo->vram,binfo->scrnx,binfo->scrny);
    mx=(binfo->scrnx-16)/2;//画面中央坐标
    my=(binfo->scrny-28-16)/2;
    init_mouse_cursor8(mcursor,COL8_008484);
    putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
    sprintf(s,"(%d,%d)",mx,my);
    putfont8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);

    sprintf(s,"memory %dMB  free:%dKB",memtotal/(1024*1024),memman_total(memman)/1024);
    putfont8_asc(binfo->vram,binfo->scrnx,0,32,COL8_FFFFFF,s);

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

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start,unsigned int end)
{
    char flag486=0;
    unsigned int eflag,cr0,i;

    eflag=io_load_eflags();//确认CPU是386还是486以上的
    eflag |=EFLAGS_AC_BIT;
    io_store_eflags(eflag);
    eflag=io_load_eflags();
    if((eflag & EFLAGS_AC_BIT)!=0)//如果是386,即时设定AC=1,AC的值还会自动回到0
    {
        flag486=1;
    }
    eflag &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflag);

    if(flag486!=0)
    {
        cr0=load_cr0();
        cr0 |= CR0_CACHE_DISABLE;//禁止缓存
        store_cr0(cr0);
    }

    i=memtest_sub(start,end);

    if(flag486!=0)
    {
        cr0=load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;//允许缓存
        store_cr0(cr0);
    }

    return i;
}

// unsigned int memtest_sub(unsigned int start,unsigned int end)
// {
//     unsigned int i,*p,old,pat0=0xAA55AA55,pat1=0x55AA55AA;
//     for(i=start;i<=end;i+=0x1000)
//     {
//         p=(unsigned int *)(i+0xFFC);
//         old=*p;
//         *p=pat0;
//         *p ^=0xFFFFFFFF;
//         if(*p!=pat1)
//         {
// NOT_MEMORY:
//             *p=old;
//             break;
//         }
//         *p ^= 0xFFFFFFFF;
//         if(*p!=pat0)
//         {
//             goto NOT_MEMORY;
//         }
//         *p=old;
//     }
//     return i;
// }

void memman_init(struct MEMMAN *man)
{
    man->frees=0;           //可用信息数目
    man->maxfrees=0;        //用于观察可用状况:frees的最大值
    man->lostsize=0;        //释放失败的内存大小总和
    man->losts=0;           //释放失败次数
    return;
}

unsigned int memman_total(struct MEMMAN *man)   //报告空余内存大小的合计
{
    unsigned int i,t=0;
    for(i=0;i<man->frees;i++)
    {
        t+=man->free[i].size;
    }
    return t;
}

unsigned int memman_alloc(struct MEMMAN *man,unsigned int size) //分配
{
    unsigned int i,a;
    for(i=0;i<man->frees;i++)
    {
        if(man->free[i].size>=size)     //找到足够大的内存
        {
            a=man->free[i].addr;
            man->free[i].addr+=size;
            man->free[i].size-=size;
            if(man->free[i].size==0)    //如果free[i]变成了0,就减掉一条可用信息
            {
                man->frees--;
                for(;i<man->frees;i++)
                {
                    man->free[i]=man->free[i+1];    //代入结构体
                }
            }
            return a;
        }
    }

    return 0;   //没有可用空间
}

int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size) //释放
{
    int i,j;
    for(i=0;i<man->frees;i++)   //为便于归纳内存,将free[]按照addr的顺序排列
    {
        if(man->free[i].addr>addr)
        {
            break;
        }
    }

    if(i>0)     //前面有可用内存
    {
        if(man->free[i-1].addr+man->free[i-1].size==addr)    //可以与前面的可用内存归纳到一起
        {
            man->free[i-1].size+=size;
            if(i<man->frees)  //后面也有
            {
                if(addr+size==man->free[i].addr)    //也可以与后面的可用内存归纳到一起
                {
                    man->free[i-1].size+=man->free[i].size;
                    man->frees--;   //man->free[i]删除,free[i]变为0归纳到前面去
                    for(;i<man->frees;i++)
                    {
                        man->free[i]=man->free[i+1];    //结构体赋值
                    }
                }
            }
            return 0;   //成功完成
        }
    }

    if(i<man->frees)    //不能与前面的可用空间归纳到一起
    {   //后面还有
        if(addr+size==man->free[i].addr)    //可以与后面的内容归纳到一起
        {
            man->free[i].addr=addr;
            man->free[i].size=size;
            return 0;   //成功完成
        }
    }

    if(man->frees<MEMMAN_FREES) //既不能与前面的归纳到一起,也不能与后面的归纳到一起
    {
        for(j=man->frees;j>i;j--)   //free[i]之后的,向后移动,腾出一点可用空间
        {
            man->free[j]=man->free[j-1];
        }
        man->frees++;
        if(man->maxfrees<man->frees)
        {
            man->maxfrees=man->frees;   //更新最大值
        }
        man->free[i].addr=addr;
        man->free[i].size=size;
        return 0;   //成功完成
    }

    man->losts++;   //不能向后移动
    man->lostsize+=size;
    return -1;  //失败
}
