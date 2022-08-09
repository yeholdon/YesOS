;%define    _BOOT_DEBUG_    ;实际的Boot Sector要注释它，这个是用来做.com文件方便调试用的

%ifdef  _BOOT_DEBUG_
    org 0100h   ;用DOS加载时规定的位置，调试时用
%else
    org 07c00h  ;Boot状态，BIOS把Boot Sector加载到0:7c00处开始执行
%endif

;常量定义区
;=====================================================================
%ifdef  _BOOT_DEBUG_
BaseOfStack equ 0100h   ;调试状态（借助DOS）下的栈底
%else
BaseOfStack equ 07c00h  ;正常加载执行状态下的，堆栈段底
%endif
    
StartSectorNoOfRootDir	equ	19	; 根目录所在首扇区号,循环初值
RootDirSectors	equ	14			; 根目录占用的扇区数
DeltaSectorNo	equ	17			; 文件真正的起始sector号 = DirEntry中的开始Sector号 + 根目录占用Sector数目 + DeltaSectorNo	
SectorNoOfFAT1	equ	1			; FAT1 的第一个扇区号 = BPB_RsvdSecCnt

BaseOfLoaderInMemery	equ	09000h	;loader.bin 被加载到的位置段基址
OffsetOfLoaderInMemery	equ	0100h	;loader.bin 被加载到内存的偏移地址

    jmp short   LABEL_START
    nop         ;这两行是FAT12格式规定的，上面的跳转说明处理器从头开始执行

	; 下面是 FAT12 磁盘的头
	BS_OEMName	DB 'ForrestY'	; OEM String, 必须 8 个字节
	BPB_BytsPerSec	DW 512		; 每扇区字节数
	BPB_SecPerClus	DB 1		; 每簇多少扇区
	BPB_RsvdSecCnt	DW 1		; Boot 记录占用多少扇区
	BPB_NumFATs	DB 2		; 共有多少 FAT 表
	BPB_RootEntCnt	DW 224		; 根目录文件数最大值
	BPB_TotSec16	DW 2880		; 逻辑扇区总数
	BPB_Media	DB 0xF0		; 媒体描述符
	BPB_FATSz16	DW 9		; 每FAT扇区数
	BPB_SecPerTrk	DW 18		; 每磁道扇区数
	BPB_NumHeads	DW 2		; 磁头数(面数)
	BPB_HiddSec	DD 0		; 隐藏扇区数
	BPB_TotSec32	DD 0		; 如果 wTotalSectorCount 是 0 由这个值记录扇区数
	BS_DrvNum	DB 0		; 中断 13 的驱动器号
	BS_Reserved1	DB 0		; 未使用
	BS_BootSig	DB 29h		; 扩展引导标记 (29h)
	BS_VolID	DD 0		; 卷序列号
	BS_VolLab	DB 'OrangeS0.02'; 卷标, 必须 11 个字节
	BS_FileSysType	DB 'FAT12   '	; 文件系统类型, 必须 8个字节  

LABEL_START:
    ; 先初始化各个段寄存器,这里就一个段。但是要注意堆栈段的段基址和栈顶的设置。
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BaseOfStack

	; 清屏，使用BIOS 10h中断（屏幕显示中断），AH=6表示把指定的矩形区域向上移动
	; 后面的补上的内容可以自行设置，黑底白字空格就相当于清屏
	mov ax, 0600h	; AH=6, AL=0(上卷行数为0表示矩形区域所有列都上卷)
	mov bx, 0700h	; 补上的内容属性BH，BH=07h黑底白字
	mov cx, 0		; CX高低字节表示左上角行列号(0,0)
	mov dx, 0184fh	; DX高低字节表示右下角行列号(79,49)
	int 10h			; 屏幕显示中断

	; 显示一个字符串，表示下面进行引导过程
	; 'Booting'
	mov dh, 0
	call	DispStr

	xor	ah, ah	; ┓
	xor	dl, dl	; ┣ 软驱复位
	int	13h		; int 13h, AH=0,软盘系统复位,后面再开始读软盘
	
	; 下面正式开始引导步骤，先是查找Loader.bin在根目录区的位置
	mov word [wSectorNo], StartSectorNoOfRootDir	; 初始化第一个查找的扇区号，19
LABEL_SEARCH_IN_ROOT_DIR_PER_SECTOR:		;最外层循环的标号，扇区级
	cmp word [wRootDirSizeForLoop], 0		;判断循环边界
	jz	LABEL_NOT_FOUND_LOADER_BIN			;没有这个文件
	dec	word [wRootDirSizeForLoop]

	;读取并查找一个扇区
	;先设置好读取到的内存位置、读取的扇区号
	mov ax, BaseOfLoaderInMemery
	mov es, ax		;es:bx是目标逻辑地址
	mov bx, OffsetOfLoaderInMemery
	mov ax, [wSectorNo]	;读取的目标扇区号
	mov cl, 1			;读取扇区数
	call ReadSector		;读取一个前面的扇区

	;读完后开始循环找根目录项中文件名条目为LOADER BIN的
	mov si, LoaderFileName	; ds:si 指向目标字符串“LOADER BIN”
	mov di, OffsetOfLoaderInMemery ;es:di 指向读入的根目录扇区
	cld
	mov dx, 10h					;一个根目录项占32字节，一个扇区512字节，所以循环次数为10h，16次
;下面开始循环遍历根目录项
LABEL_SEARCH_IN_A_SECTOR_OF_ROOT_DIR:
	cmp dx, 0
	jz LABEL_GO_TO_NEXT_SECTOR_IN_ROOT_DIR	;已经查找完一个扇区，更新循环变量，然后进入下一扇区
	;判断当前该目录项是否是目标文件名的目录项
	dec dx			;更新第2层循环变量
	mov cx, 11		;第三层循环变量，一个目录项的文件名部分11个字符

	;第3层循环，判断11个字符是否相符
LABEL_CMP_FILENAME:
	cmp cx, 0
	jz	LABEL_FILENAME_FOUND	;11个字符都匹配
	dec cx
	lodsb		;ds:si -> 把一个目标字符加载到 al中
	;判断当前字符
	cmp  al, byte [es:di]	;di不断递增
	jz	LABEL_GO_ON_NEXT_CHAR	;相同，继续比较下一个，但要先更新循环变量
	;当前字符不匹配，直接比较下一个根目录项。
	jmp LABEL_GO_ON_NEXT_FILENAME
LABEL_GO_ON_NEXT_CHAR:
	inc di					;si每次lodsb的时候会自动递增,所以只要更新di
	jmp LABEL_CMP_FILENAME	;继续比较下一个字符
LABEL_GO_ON_NEXT_FILENAME:
	;在继续比较下一个根目录项之前，先更新一下目录项的指针(di)，以便能够正确找到下一个
	and di, 0FFE0h	;因为上一个条目内的偏移不一定，所以先退会到条目首地址
	add di, 20h		;再转到下一个条目开头
	mov	si, LoaderFileName
	jmp LABEL_SEARCH_IN_A_SECTOR_OF_ROOT_DIR

LABEL_GO_TO_NEXT_SECTOR_IN_ROOT_DIR:
	;查完一个扇区没找到，进入下一个扇区
	;先更新下一个要读取的扇区号
	add word [wSectorNo], 1
	jmp	LABEL_SEARCH_IN_ROOT_DIR_PER_SECTOR



LABEL_NOT_FOUND_LOADER_BIN:
	;遍历完整个根目录都没找到，输出提示信息，然后先死循环
	mov dh, 2		;字符串常量表的第2项
	call DispStr
%ifdef	_BOOT_DEBUG_
	mov ax, 4c00h		;用BIOS调试模式
	int 21h				;没有找到LOADER.BIN 通过BIOS中断回到DOS
%else
	jmp $				;否则正常情况，死循环在这里
%endif

LABEL_FILENAME_FOUND:
	;找到是正常情况，放最后，可以接着执行后面的内容
	;找到后，做的就是读取FAT，根据里的文件扇区链表，然后依次读取文件的内容所在扇区
	
	mov ax, RootDirSectors
	and	di, 0FFE0h		;di回到当前根目录项的开始
	add di, 01Ah			;条目内偏移1Ah处是此条目对应的开始簇号（这里就是扇区号）

	mov cx, word [es:di]
	push cx				;把这个loader.bin文件所在开始扇区号保存一下
	;这个扇区号是从2开始的，要转成真正的相对于软盘开头的扇区号
	add cx, ax	;ax里存的是根目录占的扇区数
	add cx, DeltaSectorNo
	
	;设置好读入存放的逻辑地址，读取FAT扇区，再根据FAT条目读入其余扇区
	mov	ax, BaseOfLoaderInMemery
	mov es, ax		;es <- BaseOfLoaderInMemery
	mov bx, OffsetOfLoaderInMemery
	mov ax, cx		;ax <- Sector号,作为ReadSector函数的参数

LABEL_GO_ON_LOADING_FILE:
	;读FAT扇区和读文件扇区是同步进行的

	;先搞点花样，让读取过程可见
	push	ax			; `.
	push	bx			;  |
	mov	ah, 0Eh			;  | 每读一个扇区就在 "Booting  " 后面
	mov	al, '.'			;  | 打一个点, 形成这样的效果:
	mov	bl, 0Fh			;  | Booting ......
	int	10h			;  |
	pop	bx			;  |
	pop	ax			; /

	;下面进行正式读取
	mov cl, 1
	call	ReadSector
	pop ax				;取出原来存的根目录条目中找到的LOADER.BIN的扇区号，作为参数，获取目标文件的下一个扇区号
	call	GetFATEntry	;获取目标的文件扇区号对应的FAT项
	cmp	ax, 0FFFh		;判断是否是最后一个扇区
	jz	LABEL_FILE_LOADED
	;没到文件的最后一个扇区，继续读取
	push ax				;保存获取到的PAT项，也就是要读取的下一个扇区的扇区号
	
	;转换成全局的扇区号（也就是相对于软盘开头的）
	mov dx, RootDirSectors
	add ax, dx
	add ax, DeltaSectorNo

	;计算下一个扇区放置在内存中的位置——bx的设置
	add bx, [BPB_BytsPerSec]
	jmp LABEL_GO_ON_LOADING_FILE	;继续读下一个扇区

LABEL_FILE_LOADED:
	;整个文件的所有扇区都已读完
	mov dh, 1		;显示“Ready”字符串，表示Bootloader读取完毕
	call DispStr

;****************************************************
	;最后，跳到加载的BootLoader处，将控制权转交给Loader
	jmp BaseOfLoaderInMemery:OffsetOfLoaderInMemery
	; 这一句正式跳转到已加载到内
	; 存中的 LOADER.BIN 的开始处，
	; 开始执行 LOADER.BIN 的代码。
	; Boot Sector 的使命到此结束。
	; 段间远转移，还没开启保护模式和分页
;*************************************************************
	

;============================================================================
;变量, 有的作为循环变量，有的是循环中要改变的。循环比较复杂，层数比较多，统一用cmp+jmp会比
;清晰一些，loop在多层循环时会很复杂
;----------------------------------------------------------------------------
wRootDirSizeForLoop	dw	RootDirSectors	; Root Directory 占用的扇区数, 在循环中会递减至零.
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数

;============================================================================
;字符串， 用来显示或者查找的，常量
;----------------------------------------------------------------------------
LoaderFileName		db	"LOADER  BIN", 0	; LOADER.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
BootMessage:		db	"Booting  "; 9字节, 不够则用空格补齐. 序号 0
Message1		db	"Ready.   "; 9字节, 不够则用空格补齐. 序号 1
Message2		db	"No LOADER"; 9字节, 不够则用空格补齐. 序号 2
;============================================================================

;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based),就一个参数，用寄存器即可
; 同样，也是用int 10h
DispStr:
	mov ax, MessageLength
	mul dh	; 在ax得到待显示字符在字符串表中的偏移
	add ax, BootMessage
	mov bp, ax	;ES:BP=串地址
	mov ax, ds
	mov es, ax

	mov cx, MessageLength	;CX=串长度
	mov ax, 1301h			;AH=13h表示功能为显示字符串，AL=01h表示
							;光标跟随移动，所有字符统一为BL表示的属性
	mov bx, 0007h			;BH=0表示页号，BL=07H，表示黑底白字
	mov dl, 0				;DH，DL表示起始的行列
	int 10h					
	ret

;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;	从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                          ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数       │
	;                   └ 余 z => 起始扇区号 = z + 1
    ; 用堆栈暂存一些寄存器传入的参数，寄存器空出来先完成别的计算任务。
	push	bp
	mov bp, sp		;这里用bp的而不直接用sp的原因是：sp用mov的话是向下扩展的
	sub esp, 2		;开辟出两个字节暂存传入的读取扇区数参数
	mov byte [bp-2], cl
	;下面开始转换扇区号
	push	bx		;bx是参数，而且后面除法有用，先存一下
	mov bl, [BPB_SecPerTrk]	;除数，引导扇区里的项：每磁道扇区数
	div bl					;y在al,z在ah中
	inc ah					;起始扇区号 = z + 1
	;用dh,dl,ch,cl来存磁头号，驱动器号（0表示A盘），柱面号和起始扇区号
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到
	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
.GoOnReading:			;下面正式开始读取
	mov ah, 2			;AH=2，表示读磁盘（给的参数已经转成磁盘参数了
	mov al, byte [bp-2]	;读取的扇区数
	
	int 13h				;表示功能为显示字符串，AL
	jc	.GoOnReading	;如果读取错误CF会被置1，不停读，直到正确

	add esp, 2			;在栈中临时暂存的内容删除，栈指针恢复
	pop bp

	ret

;----------------------------------------------------------------------------
; 函数名: GetFATEntry
;----------------------------------------------------------------------------
; 作用:
;	找到序号为 ax 的 Sector 在 FAT 中的条目, 结果放在 ax 中
;	需要注意的是, 中间需要读 FAT 的扇区到 es:bx 处, 所以函数一开始保存了 es 和 bx
GetFATEntry:
	push	es
	push	bx
	push	ax
	mov ax, BaseOfLoaderInMemery
	sub ax, 0100h		;在BaseOfLoader前面留4KB空间用来存FAT
	mov	es, ax
	pop	ax				;数据区第一个簇的簇号是2

	;下面计算ax也就是扇区号的奇偶性。ax从2开始算，单数其实是第偶数个扇区，
	;比如ax=3代表从1开始算的第4个扇区，3*3/2=4，
	;同时将扇区号转换成该扇区对应的FAT条目在FAT中的字节偏移值
	mov byte [bOdd], 0	;默认是偶数，后面计算如果是奇数再改
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	cmp dx, 0
	jz	LABEL_EVEN
	mov byte [bOdd], 1		;奇数，后面还有用，先存一下
LABEL_EVEN:
	;假设扇区号是2(第3个扇区),2*3/2=3，得到的字节偏移量是3，从0开始算的话就是
	;第4个字节是第3个扇区对应的PAT项的开始偏移量，这没错，因为前两个扇区对应的
	;PAT项每个占12位，共3字节。

	;下面来计算FAT项在那个扇区中，很简单只要字节偏移量/每个扇区的字节数(512)即可
	xor	dx, dx
	mov bx, [BPB_BytsPerSec]
	div bx ; dx:ax / BPB_BytsPerSec
		   ;  ax <- 商 (FATEntry 所在的扇区相对于 FAT 的扇区号)
		   ;  dx <- 余数 (FATEntry 在扇区内的偏移)。
	push dx
	mov bx, 0
	add ax, SectorNoOfFAT1	;因为FAT1前面还有一个引导扇区，绝对扇区号要加上1
	mov cl, 2				;因为一个FAT项可能跨两个扇区，所以每次读两个扇区
	call	ReadSector		;读到前面预留的BaseOfLoader前面4KB空间
							;读第ax号扇区到es:bx，es前面就设置好了
	pop dx					;现在取出目标PAT项在扇区内的偏移
	add bx, dx				;加上段内偏移
	mov ax, [es:bx]			;获取PAT项，也就是下一个文件扇区的扇区号,12位，所以用ax读16位出来

	cmp byte [bOdd], 0		;偶数，那就是16位里的低12位
	jz	LABEL_EVEN_2
	shr	ax, 4				;奇数，就是16位里的高12位1

LABEL_EVEN_2:
	and ax, 0FFFh

LABEL_GET_FAT_ENRY_OK:		;完成读取对应扇区的FAT项

	pop	bx
	pop	es
	ret
;--------------------------------------------------
times	510-($-$$)	db	0	;填充剩余空间，使生成的二进制文件大小正好为512字节
dw	0xaa55					;结束标志，2字节
