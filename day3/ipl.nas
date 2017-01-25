;EasyOS day2

    CYLS EQU 10
    ORG 0x7C00  ;指明程序的装载地址

;标准FAT12格式软盘的专用代码
    JMP ENTRY
    DB 0x90
    DB "EASYOS  ";启动区的名称(8字节)
    DW 512       ;每个扇区大小(必须512字节)
    DB 1         ;簇大小(必须1扇区)
    DW 1         ;FAT的起始位置(一般从第一个扇区开始)
    DB 2         ;FAT的个数(必须为2)
    DW 224       ;根目录的大小(一般设成224项)
    DW 2880      ;该磁盘的大小(必须时2880扇区)
    DB 0xF0      ;该磁盘的种类(必须是0XF0)
    DW 9         ;FAT的长度(必须是9扇区)
    DW 18        ;1个磁道有几个扇区(必须是18)
    DW 2         ;磁头数(必须是2)
    DW 2         ;不使用分区,必须是0
    DD 2880      ;重写一次磁盘大小
    DB 0,0,0x29  ;意义不明,固定
    DD 0xFFFFFFFF;(可能是)卷标号码
    DB "EASYOS     ";磁盘的名称(11字节)
    DB "FAT12   ";磁盘格式名称(8字节)
    TIMES 18 DB 0     ;先空出18字节

;程序主体
ENTRY:
    MOV AX,0    ;初始化寄存器
    MOV SS,AX
    MOV SP,0x7C00
    MOV DS,AX

;读磁盘
    MOV AX,0x0820
    MOV ES,AX
    MOV CH,0    ;柱面0
    MOV DH,0    ;磁头0
    MOV CL,2    ;扇区2

READLOOP:
    MOV SI,0    ;记录失败次数的寄存器
RETRY:
    MOV AH,0x02 ;AH=0x02:读盘
    MOV AL,1    ;1个扇区
    MOV BX,0
    MOV DL,0x00 ;A驱动器
    INT 0x13    ;调用磁盘BIOS
    JNC NEXT    ;没出错的话跳转到FIN
    INC SI      ;加一
    CMP SI,5    ;和5比较
    JAE ERROR   ;SI>=5,跳转到ERROR
    MOV AH,0x00
    MOV DL,0x00 ;A驱动器
    INT 0x13    ;重置驱动器
    JMP RETRY

NEXT:
    MOV AX,ES   ;把内存地址后移0x20
    ADD AX,0x0020
    MOV ES,AX
    INC CL    ;读下一扇区
    CMP CL,18   ;读到18扇区
    JBE READLOOP
    MOV CL,1
    ADD DH,1
    CMP DH,2
    JB READLOOP ;读反面的柱面
    MOV DH,0
    ADD CH,1
    CMP CH,CYLS
    JB READLOOP ;读取10个柱面

    MOV [0x0FF0],CH
    JMP 0xC400

FIN:
    HLT         ;让CPU停止,等待命令
    JMP FIN     ;无限循环
ERROR:
    MOV SI,MSG
PUTLOOP:
    MOV AL,[SI]
    ADD SI,1
    CMP AL,0
    JE FIN
    MOV AH,0x0E ;显示一个文字
    MOV BX,15   ;指定字符颜色
    INT 0x10    ;调用显卡BIOS
    JMP PUTLOOP
MSG:
    DB 0x0A,0x0A;换行两次
    DB "Load error"
    DB 0x0A     ;换行
    DB 0

    TIMES 510-($-$$) DB 0;填写0x00,直到0x7DFE
    DB 0x55,0xAA
