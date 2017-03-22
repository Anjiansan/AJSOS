#include "bootpack.h"
#include <stdio.h>
#include <string.h>

void console_task(struct SHEET *sheet,unsigned int memtotal)
{
    struct TIMER *timer;
    struct TASK *task=task_now();
    int i,fifobuf[128],cursor_x=16,cursor_y=28,cursor_c=-1,x,y;
    char s[30],cmdline[30],*p;
    struct MEMMAN *memman=(struct MEMMAN *)MEMMAN_ADDR;
    struct FILEINFO *finfo=(struct FILEINFO *)(ADR_DISKIMG+0x2600);
    int *fat=(int *)memman_alloc_4k(memman,4*2880);
    struct SEGMENT_DESCRIPTOR *gdt=(struct SEGMENT_DESCRIPTOR *)ADR_GDT;

    fifo32_init(&task->fifo,128,fifobuf,task);
    timer=timer_alloc();
    timer_init(timer,&task->fifo,1);
    timer_settime(timer,50);
    file_readfat(fat,(unsigned char *)(ADR_DISKIMG+0x000200));

    //显示提示字符
    putfont8_asc_sht(sheet,8,28,COL8_FFFFFF,COL8_000000,">",1);

    for(;;)
    {
        io_cli();
        if(fifo32_status(&task->fifo)==0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            i=fifo32_get(&task->fifo);
            io_sti();
            if(i<=1)    //光标
            {
                if(i!=0)
                {
                    timer_init(timer,&task->fifo,0);   //然后设置0
                    if(cursor_c>=0)
                    {
                        cursor_c=COL8_FFFFFF;
                    }
                }
                else
                {
                    timer_init(timer,&task->fifo,1);   //然后设置1
                    if(cursor_c>=0)
                    {
                        cursor_c=COL8_000000;
                    }
                }
                timer_settime(timer,50);
            }
            if(i==2)    //光标ON
            {
                cursor_c=COL8_FFFFFF;
            }
            if(i==3)    //光标OFF
            {
                boxfill8(sheet->buf,sheet->bxsize,COL8_000000,cursor_x,28,cursor_x+7,43);
                cursor_c=-1;
            }
            if(256<=i && i<=511)    //键盘数据
            {
                if(i==8+256)    //退格键
                {
                    if(cursor_x>16)
                    {
                        putfont8_asc_sht(sheet,cursor_x,cursor_y,COL8_FFFFFF,COL8_000000," ",1);
                        cursor_x-=8;
                    }
                }
                else if(i==10+256)   //回车键
                {
                    putfont8_asc_sht(sheet,cursor_x,cursor_y,COL8_FFFFFF,COL8_000000," ",1);    //用空格将光标擦除
                    cmdline[cursor_x/8-2]=0;
                    cursor_y=cons_newline(cursor_y,sheet);
                    if(strcmp(cmdline,"free")==0)
                    {   //free命令
                        sprintf(s,"total %dMB",memtotal/(1024*1024));
                        putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,s,30);
                        cursor_y=cons_newline(cursor_y,sheet);
                        sprintf(s,"free %dKB",memman_total(memman)/1024);
                        putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,s,30);
                        cursor_y=cons_newline(cursor_y,sheet);
                        cursor_y=cons_newline(cursor_y,sheet);
                    }
                    else if(strcmp(cmdline,"cls")==0)
                    {   //cls命令
                        for(y=28;y<28+128;y++)
                        {
                            for(x=8;x<8+240;x++)
                            {
                                sheet->buf[x+y*sheet->bxsize]=COL8_000000;
                            }
                        }
                        sheet_refresh(sheet,8,28,8+240,28+128);
                        cursor_y=28;
                    }
                    else if(strcmp(cmdline,"ls")==0)
                    {   //ls
                        for(x=0;x<240;x++)
                        {
                            if(finfo[x].name[0]==0x00)
                            {
                                break;
                            }
                            if(finfo[x].name[0]!=0xE5)
                            {
                                if((finfo[x].type & 0x18)==0)
                                {
                                    sprintf(s,"filename.ext %7d",finfo[x].size);
                                    for(y=0;y<8;y++)
                                    {
                                        s[y]=finfo[x].name[y];
                                    }
                                    s[9]=finfo[x].ext[0];
                                    s[10]=finfo[x].ext[1];
                                    s[11]=finfo[x].ext[2];
                                    putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,s,30);
                                    cursor_y=cons_newline(cursor_y,sheet);
                                }
                            }
                        }
                        cursor_y=cons_newline(cursor_y,sheet);
                    }
                    else if(strncmp(cmdline,"cat ",4)==0)
                    {   //cat命令
                        for(y=0;y<11;y++)   //准备文件名
                        {
                            s[y]=' ';
                        }
                        y=0;
                        for(x=4;y<11 && cmdline[x]!=0;x++)
                        {
                            if(cmdline[x]=='.' && y<=8)
                            {
                                y=8;
                            }
                            else
                            {
                                s[y]=cmdline[x];
                                if('a'<=s[y] && s[y]<='z')
                                {   //小写转大写
                                    s[y]-=0x20;
                                }
                                y++;
                            }
                        }
                        for(x=0;x<240;) //寻找文件
                        {
                            if(finfo[x].name[0]==0x00)
                            {
                                break;
                            }
                            if((finfo[x].type & 0x18)==0)
                            {
                                for(y=0;y<11;y++)
                                {
                                    if(finfo[x].name[y]!=s[y])
                                    {
                                        goto TYPE_NEXT_FILE;
                                    }
                                }
                                break;  //找到文件
                            }
TYPE_NEXT_FILE:             x++;
                        }
                        if(x<240 && finfo[x].name[0]!=0x00)
                        {   //找到文件时
                            p=(char *)memman_alloc_4k(memman,finfo[x].size);
                            file_loadfile(finfo[x].clustno,finfo[x].size,p,fat,(char *)(ADR_DISKIMG+0x003E00));
                            cursor_x=8;
                            for(y=0;y<finfo[x].size;y++)
                            {
                                s[0]=p[y];
                                s[1]=0;
                                if(s[0]==0x09)   //制表符
                                {
                                    for(;;)
                                    {
                                        putfont8_asc_sht(sheet,cursor_x,cursor_y,COL8_FFFFFF,COL8_000000," ",1);
                                        cursor_x+=8;
                                        if(cursor_x==8+240) //换行
                                        {
                                            cursor_x=8;
                                            cursor_y=cons_newline(cursor_y,sheet);
                                        }
                                        if(((cursor_x-8) & 0x1F)==0)
                                        {
                                            break;  //被32整除则break
                                        }
                                    }
                                    memman_free_4k(memman,(int)p,finfo[x].size);
                                }
                                else if(s[0]==0x0A) //换行
                                {
                                    cursor_x=8;
                                    cursor_y=cons_newline(cursor_y,sheet);
                                }
                                else if(s[0]==0x0D) //回车
                                {
                                    ;
                                }
                                else    //一般字符
                                {
                                    putfont8_asc_sht(sheet,cursor_x,cursor_y,COL8_FFFFFF,COL8_000000,s,1);
                                    cursor_x+=8;
                                    if(cursor_x==8+240) //换行
                                    {
                                        cursor_x=8;
                                        cursor_y=cons_newline(cursor_y,sheet);
                                    }
                                }
                            }
                        }
                        else
                        {   //没有找到文件
                            putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,"File not found.",15);
                            cursor_y=cons_newline(cursor_y,sheet);
                        }
                        cursor_y=cons_newline(cursor_y,sheet);
                    }
                    else if(strcmp(cmdline,"hlt")==0)
                    {   //hlt.ajs应用程序
                        for(y=0;y<11;y++)
                        {
                            s[y]=' ';
                        }
                        s[0]='H';
                        s[1]='L';
                        s[2]='T';
                        s[8]='A';
                        s[9]='J';
                        s[10]='S';
                        for(x=0;x<240;)
                        {
                            if(finfo[x].name[0]==0x00)
                            {
                                break;
                            }
                            if((finfo[x].type & 0x18)==0)
                            {
                                for(y=0;y<11;y++)
                                {
                                    if(finfo[x].name[y]!=s[y])
                                    {
                                        goto HLT_NEXT_FILE;
                                    }
                                }
                                break;  //找到文件
                            }
HLT_NEXT_FILE:
                            x++;
                        }
                        if(x<240 && finfo[x].name[0]!=0x00)
                        {
                            p=(char *)memman_alloc_4k(memman,finfo[x].size);
                            file_loadfile(finfo[x].clustno,finfo[x].size,p,fat,(char *)(ADR_DISKIMG+0x003E00));
                            set_segmdsc(gdt+1003,finfo[x].size-1,(int)p,AR_CODE32_ER);
                            farjump(0,1003*8);
                            memman_free_4k(memman,(int)p,finfo[x].size);
                        }
                        else
                        {
                            putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,"File not found.",15);
                            cursor_y=cons_newline(cursor_y,sheet);
                        }
                        cursor_y=cons_newline(cursor_y,sheet);
                    }
                    else if(cmdline[0]!=0)  //不是命令,也不是空行
                    {
                        putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,"Bad commmand",12);
                        cursor_y=cons_newline(cursor_y,sheet);
                        cursor_y=cons_newline(cursor_y,sheet);
                    }
                    putfont8_asc_sht(sheet,8,cursor_y,COL8_FFFFFF,COL8_000000,">",1);
                    cursor_x=16;    //显示提示符
                }
                else
                {
                    if(cursor_x<240)
                    {
                        s[0]=i-256;
                        s[1]=0;
                        cmdline[cursor_x/8-2]=i-256;
                        putfont8_asc_sht(sheet,cursor_x,cursor_y,COL8_FFFFFF,COL8_000000,s,1);
                        cursor_x+=8;
                    }
                }
            }
            if(cursor_c>=0)
            {
                boxfill8(sheet->buf,sheet->bxsize,cursor_c,cursor_x,cursor_y,cursor_x+7,cursor_y+15);
            }
            sheet_refresh(sheet,cursor_x,cursor_y,cursor_x+8,cursor_y+16);
        }
    }
}

int cons_newline(int cursor_y,struct SHEET *sheet)
{
    int x,y;
    if(cursor_y<28+112)
    {
        cursor_y+=16;   //换行
    }
    else
    {   //滚动
        for(y=28;y<28+112;y++)
        {
            for(x=8;x<8+240;x++)
            {
                sheet->buf[x+y*sheet->bxsize]=sheet->buf[x+(y+16)*sheet->bxsize];
            }
        }
        for(y=28+112;y<28+128;y++)
        {
            for(x=8;x<8+240;x++)
            {
                sheet->buf[x+y*sheet->bxsize]=COL8_000000;
            }
        }
        sheet_refresh(sheet,8,28,8+240,28+128);
    }
    return cursor_y;
}
