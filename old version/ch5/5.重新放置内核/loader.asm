;这里就无需加载到0x7c00了，可以是自己设置的任何位置
org 0100h   

    jmp short   LABEL_START
    ;nop        ;这两行是FAT12格式规定的，上面的跳转说明处理器从头开始执行

; 主要是fat12磁盘头的信息和相关的常量
%include	"fat12hdr.inc"
%include	"load.inc"
%include	"pm.inc"

; GDT
;                            段基址     段界限, 属性
LABEL_GDT:	    Descriptor 0,            0, 0              ; 空描述符
LABEL_DESC_FLAT_C:  Descriptor 0,      0fffffh, DA_CR|DA_32|DA_LIMIT_4K ;0-4G
LABEL_DESC_FLAT_RW: Descriptor 0,      0fffffh, DA_DRW|DA_32|DA_LIMIT_4K;0-4G
LABEL_DESC_VIDEO:   Descriptor 0B8000h, 0ffffh, DA_DRW|DA_DPL3 ; 显存首地址

GdtLen		equ	$ - LABEL_GDT
GdtPtr		dw	GdtLen - 1				; 段界限
		dd	BaseOfLoaderPhyAddr + LABEL_GDT		; 基地址

; GDT 选择子
SelectorFlatC		equ	LABEL_DESC_FLAT_C	- LABEL_GDT
SelectorFlatRW		equ	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo		equ	LABEL_DESC_VIDEO	- LABEL_GDT + SA_RPL3



BaseOfStack equ 0100h   ;栈底，和加载位置一样，因为栈的扩展方向不同
;增加了页表和页目录的起始地址两个常量





LABEL_START:
    ; 先初始化各个段寄存器,这里就一个段。但是要注意堆栈段的段基址和栈顶的设置。
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BaseOfStack

	; 这里就不清屏了，为了显示完整的启动过程

	; 显示一个字符串，表示下面进行引导过程
	; 'loading',字符串表内容变了
	mov dh, 0
	call	DispStrRealMode


	; 获取可用内存信息，供后面保护模式分页使用，在实模式下用中断方便，所以放在前面获取
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf		; es:di 指向一个地址范围描述符结构(ARDS)
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h		; edx = 'SMAP'
	int	15h			; int 15h
	jc	.MemChkFail
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
	mov	dword [_dwMCRNumber], 0
.MemChkOK:


	xor	ah, ah	; ┓
	xor	dl, dl	; ┣ 软驱复位
	int	13h		; int 13h, AH=0,软盘系统复位,后面再开始读软盘
	
	; 下面正式开始引导步骤，先是查找Loader.bin在根目录区的位置
	mov word [wSectorNo], StartSectorNoOfRootDir	; 初始化第一个查找的扇区号，19
LABEL_SEARCH_IN_ROOT_DIR_PER_SECTOR:		;最外层循环的标号，扇区级
	cmp word [wRootDirSizeForLoop], 0		;判断循环边界
	jz	LABEL_NOT_FOUND_KERNEL_BIN			;没有这个文件
	dec	word [wRootDirSizeForLoop]

	;读取并查找一个扇区
	;先设置好读取到的内存位置、读取的扇区号
	mov ax, BaseOfKernelFile
	mov es, ax		;es:bx是目标逻辑地址
	mov bx, OffsetOfKernelFile
	mov ax, [wSectorNo]	;读取的目标扇区号
	mov cl, 1			;读取扇区数
	call ReadSector		;读取一个前面的扇区

	;读完后开始循环找根目录项中文件名条目为LOADER BIN的
	mov si, KernelFileName	; ds:si 指向目标字符串“KERNEL BIN”
	mov di, OffsetOfKernelFile ;es:di 指向读入的根目录扇区
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
	mov	si, KernelFileName
	jmp LABEL_SEARCH_IN_A_SECTOR_OF_ROOT_DIR

LABEL_GO_TO_NEXT_SECTOR_IN_ROOT_DIR:
	;查完一个扇区没找到，进入下一个扇区
	;先更新下一个要读取的扇区号
	add word [wSectorNo], 1
	jmp	LABEL_SEARCH_IN_ROOT_DIR_PER_SECTOR


LABEL_NOT_FOUND_KERNEL_BIN:
	;遍历完整个根目录都没找到，输出提示信息，然后先死循环
	mov dh, 2		;字符串常量表的第2项
	call DispStrRealMode

	jmp $				;没找到，死循环在这里


LABEL_FILENAME_FOUND:
	;找到是正常情况，放最后，可以接着执行后面的内容
	;找到后，做的就是读取FAT，根据里的文件扇区链表，然后依次读取文件的内容所在扇区
	
	mov ax, RootDirSectors
	and	di, 0FFE0h		;di回到当前根目录项的开始
	

    ;相比boot.asm增加的，保存kernel.bin文件的大小
    push eax
    mov eax, [es:di + 01Ch]
    mov dword   [dwKernelSize], eax
    pop eax

    add di, 01Ah			;条目内偏移1Ah处是此条目对应的开始簇号（这里就是扇区号）
	mov cx, word [es:di]
	push cx				;把这个kernel.bin文件所在开始扇区号保存一下
	;这个扇区号是从2开始的，要转成真正的相对于软盘开头的扇区号
	add cx, ax	;ax里存的是根目录占的扇区数
	add cx, DeltaSectorNo
	
	;设置好读入存放的逻辑地址，读取FAT扇区，再根据FAT条目读入其余扇区
	mov	ax, BaseOfKernelFile
	mov es, ax		;es <- BaseOfKernelFile
	mov bx, OffsetOfKernelFile
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
    ;关闭马达驱动，不然软驱灯会一直亮着（这个其实虚拟机每影响）
    call KillMotor

	;整个文件的所有扇区都已读完
	mov dh, 1		;显示“Ready”字符串，表示Kernel读取完毕
	call DispStrRealMode	;这个实模式下的函数也加了个后缀，表示实模式下的显示函数，保护模式下也有，不能重复


;****************************************************
	;下面就是准备跳入保护模式的部分
	;加载GDTR
	lgdt	[GdtPtr]

	;关中断
	cli

	;打开地址线A20
	in al, 92h
	or al, 00000010b
	out	92h, al

	;打开CR0的PM位
	mov	eax, cr0
	or eax, 1
	mov	cr0, eax

	;正式进入保护模式,平坦模式就一个段，段基址为0，所以物理地址就是偏移地址
	jmp	dword SelectorFlatC:(BaseOfLoaderPhyAddr+LABEL_PM_START)
	jmp $
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
dwKernelSize    dd  0       ;kernel.bin文件的大小

;============================================================================
;字符串， 用来显示或者查找的，常量
;----------------------------------------------------------------------------
KernelFileName		db	"KERNEL  BIN", 0	; KERNEL.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
LoadMessage:		db	"Loading  "; 9字节, 不够则用空格补齐. 序号 0
Message1		db	"Ready.   "; 9字节, 不够则用空格补齐. 序号 1
Message2		db	"No Kernel"; 9字节, 不够则用空格补齐. 序号 2
;============================================================================

;----------------------------------------------------------------------------
; 函数名: DispStrRealMode
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based),就一个参数，用寄存器即可
; 同样，也是用int 10h
DispStrRealMode:
	mov ax, MessageLength
	mul dh	; 在ax得到待显示字符在字符串表中的偏移
	add ax, LoadMessage
	mov bp, ax	;ES:BP=串地址
	mov ax, ds
	mov es, ax

	mov cx, MessageLength	;CX=串长度
	mov ax, 1301h			;AH=13h表示功能为显示字符串，AL=01h表示
							;光标跟随移动，所有字符统一为BL表示的属性
	mov bx, 0007h			;BH=0表示页号，BL=07H，表示黑底白字
	mov dl, 0				;DH，DL表示起始的行列
    add dh, 3               ;从第三行往下显示，因为前两行，Booting已经显示,不然会覆盖
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
	mov ax, BaseOfKernelFile
	sub ax, 0100h		;在BaseOfKernelFile前面留4KB空间用来存FAT
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
	call	ReadSector		;读到前面预留的BaseOfKernelFile前面4KB空间
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


;----------------------------------------------------------------------------
; 函数名: KillMotor
;----------------------------------------------------------------------------
; 作用:
;	关闭软驱马达
KillMotor:
	push	dx
	mov	dx, 03F2h
	mov	al, 0
	out	dx, al
	pop	dx
	ret
;----------------------------------------------------------------------------


; 从此以后的代码在保护模式下执行 ----------------------------------------------------
; 32 位代码段. 由实模式跳入 ---------------------------------------------------------
[SECTION .s32]

ALIGN	32

[BITS	32]

LABEL_PM_START:
	;进入PM后首要任务，设置其余段寄存器
	mov	ax, SelectorVideo
	mov	gs, ax

	mov	ax, SelectorFlatRW
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	ss, ax
	mov	esp, TopOfStack	;这个常量定义在后面，1KB大小的栈，暂时够了

	push	szMemChkTitle
	call	DispStr
	add	esp, 4

	;前面实模式的时候，先用中断获取了可用内存信息，这里直接显示，并用来
	;设置分页，比如实际物理内存大小等,显示这些信息要用到前面写过的一些
	;函数，也就是放在lib.inc里面的。
	call	DispMemInfo
	call	SetupPaging

	;也是先输出一个字符，提示一下进入了保护模式
	mov	ah, 0Fh				; 0000: 黑底    1111: 白字
	mov	al, 'P'
	mov	[gs:((80 * 0 + 39) * 2)], ax	; 屏幕第 0 行, 第 39 列。
	
	call	InitKernel

	;jmp	$
	;***************************************************************
	jmp	SelectorFlatC:KernelEntryPointPhyAddr	; 正式进入内核 *
	;***************************************************************
	; 内存看上去是这样的：
	;              ┃                                    ┃
	;              ┃                 .                  ┃
	;              ┃                 .                  ┃
	;              ┃                 .                  ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;              ┃■■■■■■Page  Tables■■■■■■┃
	;              ┃■■■■■(大小由LOADER决定)■■■■┃
	;    00101000h ┃■■■■■■■■■■■■■■■■■■┃ PageTblBase
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;    00100000h ┃■■■■Page Directory Table■■■■┃ PageDirBase  <- 1M
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       F0000h ┃□□□□□□□System ROM□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       E0000h ┃□□□□Expansion of system ROM □□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       C0000h ┃□□□Reserved for ROM expansion□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃ B8000h ← gs
	;       A0000h ┃□□□Display adapter reserved□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;       9FC00h ┃□□extended BIOS data area (EBDA)□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;       90000h ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;       80000h ┃■■■■■■■KERNEL.BIN■■■■■■┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;       30000h ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃                                    ┃
	;        7E00h ┃              F  R  E  E            ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■┃
	;        7C00h ┃■■■■■■BOOT  SECTOR■■■■■■┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃                                    ┃
	;         500h ┃              F  R  E  E            ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□┃
	;         400h ┃□□□□ROM BIOS parameter area □□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇┃
	;           0h ┃◇◇◇◇◇◇Int  Vectors◇◇◇◇◇◇┃
	;              ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	;
	;
	;		┏━━━┓		┏━━━┓
	;		┃■■■┃ 我们使用 	┃□□□┃ 不能使用的内存
	;		┗━━━┛		┗━━━┛
	;		┏━━━┓		┏━━━┓
	;		┃      ┃ 未使用空间	┃◇◇◇┃ 可以覆盖的内存
	;		┗━━━┛		┗━━━┛
	;
	; 注：KERNEL 的位置实际上是很灵活的，可以通过同时改变 LOAD.INC 中的
	;     KernelEntryPointPhyAddr 和 MAKEFILE 中参数 -Ttext 的值来改变。
	;     比如把 KernelEntryPointPhyAddr 和 -Ttext 的值都改为 0x400400，
	;     则 KERNEL 就会被加载到内存 0x400000(4M) 处，入口在 0x400400。
	;



;后面这部分主要是给分页机制用的
;*******************************************************************

%include	"lib.inc"


; 显示内存信息 --------------------------------------------------------------
DispMemInfo:
	;要用到的寄存器先入栈保存一下
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber];for(int i=0;i<[MCRNumber];i++)//每次得到一个ARDS
.loop:				  ;{
	mov	edx, 5		  ;  for(int j=0;j<5;j++)//每次得到一个ARDS中的成员
	mov	edi, ARDStruct	  ;  {//依次显示:BaseAddrLow,BaseAddrHigh,LengthLow
.1:				  ;               LengthHigh,Type
	push	dword [esi]	  ;
	call	DispInt		  ;    DispInt(MemChkBuf[j*4]); // 显示一个成员
	pop	eax		  ;
	stosd			  ;    ARDStruct[j*4] = MemChkBuf[j*4];放到一个存单独一个ARDS的结构里，方便后面对里面的各项进行计算
	add	esi, 4		  ;
	dec	edx		  ;
	cmp	edx, 0		  ;
	jnz	.1		  ;  }
	call	DispReturn	  ;  printf("\n");

	; 累加各个可用地址段的大小，得到总的可用内存大小
	cmp	dword [dwType], 1 ;  if(Type == AddressRangeMemory)
	jne	.2		  ;  {
	mov	eax, [dwBaseAddrLow];
	add	eax, [dwLengthLow];
	cmp	eax, [dwMemSize]  ;    if(BaseAddrLow + LengthLow > MemSize)
	jb	.2		  ;
	mov	[dwMemSize], eax  ;    MemSize = BaseAddrLow + LengthLow;
.2:				  ;  }
	loop	.loop		  ;}
				  ;
	call	DispReturn	  ;printf("\n");
	push	szRAMSize	  ;
	call	DispStr		  ;printf("RAM size:");
	add	esp, 4		  ;
				  ;
	push	dword [dwMemSize] ;
	call	DispInt		  ;DispInt(MemSize);
	add	esp, 4		  ;

	pop	ecx
	pop	edi
	pop	esi
	ret
; ---------------------------------------------------------------------------

; 启动分页机制 --------------------------------------------------------------
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h	; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx		; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push	ecx		; 暂存页表个数

	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PageDirBase	; 此段首地址为 PageDirBase
	xor	eax, eax
	mov	eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.1

	; 再初始化所有页表
	pop	eax			; 页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov	edi, PageTblBase	; 此段首地址为 PageTblBase
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW
.2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2

	mov	eax, PageDirBase
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .3
.3:
	nop

	ret
; 分页机制启动完毕 ----------------------------------------------------------


; InitKernel ---------------------------------------------------------------------------------
; 将 KERNEL.BIN 的内容经过整理对齐后放到新的位置
; 遍历每一个 Program Header，根据 Program Header 中的信息来确定把什么放进内存，放到什么位置，以及放多少。
; --------------------------------------------------------------------------------------------
InitKernel:
	xor	esi, esi
	; 2Ch是根据ELF结构里每个项的大小和对齐方式算出来的
	mov	cx, word	[BaseOfKernelFilePhyAddr+2Ch] ;ELF header的e_phnum ->ecx,获取program header的数目
	movzx	ecx, cx	;ELF header里的这一项是2字节的，但是ecx的4字节的，无符号mov
	mov	esi, [BaseOfKernelFilePhyAddr + 1Ch]  ; esi <- pELFHdr->e_phoff(program header table在文件里的偏移量)
	add   esi, BaseOfKernelFilePhyAddr	;转成绝对地址

.begin:		;从Program header table里一个个处理段
	mov	eax, [esi]	;取下一项Program header
	cmp	eax, 0
	jz	.noAction	;是空的，说明已经取完一个Program header中需要的项
	;获取Program header里cpy段要的信息：段的起始偏移和段的长度
	push dword	[esi+010h]		;段的长度（大小），存进栈里作为函数参数
	mov eax, [esi+04h]			;段的起始文件内偏移
	add eax, BaseOfKernelFilePhyAddr	;加上基地址，得到绝对地址
	push	eax					;第二个参数：段的起始地址
	push	dword	[esi+08h]	;p_vaddr，第三个参数：段转移的目标虚拟地址

	call MemCpy
	add esp, 12					;返回后栈复位

.noAction:
	add	esi, 020h				;处理完一个段后，更新指针
	dec	ecx
	jnz	.begin

	ret
; InitKernel结束



; SECTION .data1 之开始 ---------------------------------------------------------------------------------------------
[SECTION .data1]

ALIGN	32

LABEL_DATA:
; 实模式下使用这些符号
; 字符串
_szMemChkTitle:	db "BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:	db "RAM size:", 0
_szReturn:	db 0Ah, 0
;; 变量
_dwMCRNumber:	dd 0	; Memory Check Result
_dwDispPos:	dd (80 * 6 + 0) * 2	; 屏幕第 6 行, 第 0 列。
_dwMemSize:	dd 0
_ARDStruct:	; Address Range Descriptor Structure
  _dwBaseAddrLow:		dd	0
  _dwBaseAddrHigh:		dd	0
  _dwLengthLow:			dd	0
  _dwLengthHigh:		dd	0
  _dwType:			dd	0
_MemChkBuf:	times	256	db	0	;一个地址段描述符20字节，可以放12个，这里足够了
;
;; 保护模式下使用这些符号，保护模式下需要使用BaseOfLoaderPhyAddr进行（标号）重定位
szMemChkTitle		equ	BaseOfLoaderPhyAddr + _szMemChkTitle
szRAMSize		equ	BaseOfLoaderPhyAddr + _szRAMSize
szReturn		equ	BaseOfLoaderPhyAddr + _szReturn
dwDispPos		equ	BaseOfLoaderPhyAddr + _dwDispPos
dwMemSize		equ	BaseOfLoaderPhyAddr + _dwMemSize
dwMCRNumber		equ	BaseOfLoaderPhyAddr + _dwMCRNumber
ARDStruct		equ	BaseOfLoaderPhyAddr + _ARDStruct
	dwBaseAddrLow	equ	BaseOfLoaderPhyAddr + _dwBaseAddrLow
	dwBaseAddrHigh	equ	BaseOfLoaderPhyAddr + _dwBaseAddrHigh
	dwLengthLow	equ	BaseOfLoaderPhyAddr + _dwLengthLow
	dwLengthHigh	equ	BaseOfLoaderPhyAddr + _dwLengthHigh
	dwType		equ	BaseOfLoaderPhyAddr + _dwType
MemChkBuf		equ	BaseOfLoaderPhyAddr + _MemChkBuf


; 堆栈就在数据段的末尾，Loader里做的工作不多，所以1KB足够了，进入内核再重新设置自己的堆栈
StackSpace:	times	1024	db	0
TopOfStack	equ	BaseOfLoaderPhyAddr + $	; 栈顶
; SECTION .data1 结束 

