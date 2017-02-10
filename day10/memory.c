#include "bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man,unsigned int size)
{
    unsigned int a;
    size=(size+0xFFF) & 0xFFFFF000;
    a=memman_alloc(man,size);
    return a;
}

int memman_free_4k(struct MEMMAN *man,unsigned int addr,unsigned int size)
{
    int i;
    size=(size+0xFFF) & 0xFFFFF000;
    i=memman_free(man,addr,size);
    return i;
}
