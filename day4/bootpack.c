// 告诉C编译器,有一个函数在别的文件里
void io_hlt(void);
void write_mem8(int addr,int data);

void HariMain(void)
{
    int i;
    char *p=(char *)0xA0000;

    // for(i=0xA0000;i<=0xAFFFF;i++)
    // {
    //     // write_mem8(i,i & 0x0F);//汇编实现
    //
    //     p=(char *)i;//C语言实现
    //     *p=i & 0x0F;
    // }

    for(i=0;i<=0xFFFF;i++)//C语言的另一种简单实现
    {
        // *(p+i)=i & 0x0F;
        p[i]=i & 0x0F;
    }

    for(;;)
    {
        io_hlt();
    }
}
