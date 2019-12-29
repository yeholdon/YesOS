/**************************************************************
 * hd.c
 * 程序功能：硬盘驱动进程
 * 修改日期：2019.12.14
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
#include "proto.h"
#include  "proc.h"
#include "global.h"
#include "keyboard.h"
#include "hd.h"
#include "string.h"

PRIVATE void    init_hd();
PRIVATE void	hd_cmd_out		(struct hd_cmd* cmd);
PRIVATE int waitfor	(int mask, int val, int timeout);
PRIVATE void	interrupt_wait		();
PRIVATE	void	hd_identify		(int drive);
PRIVATE void	print_identify_info	(u16* hdinfo);
PRIVATE void	get_part_table		(int drive, int sect_nr, struct part_ent * entry);
PRIVATE void	partition		(int device, int style);
PRIVATE void	print_hdinfo		(struct hd_info * hdi);
PRIVATE void	hd_open			(int dev);
PRIVATE void	hd_rdwt(MESSAGE *pMsg);
PRIVATE void 	hd_ioctl(MESSAGE *pMsg);
PRIVATE void hd_close(int device);

PRIVATE	u8	hd_status;                                  // 硬盘状态别的函数也要用到，全局
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];   // 两个扇区的缓冲区
PRIVATE	struct hd_info	hd_info[1];			// 硬盘信息结构体，这里只有一个硬盘

// 反过来从设备号得到
#define	DRV_OF_DEV(dev)	((dev) <= MAX_PRIM ? ((dev) / NR_PRIM_PER_DRIVE) :  ((dev) - MINOR_hd1a)  / (u32)NR_SUB_PER_DRIVE)


/*****************************************************************************
 *                                task_hd
 *****************************************************************************/
/**
 * Main loop of HD driver.硬盘驱动的主循环，初始化后不断循环接收消息
 * 
 *****************************************************************************/

PUBLIC void task_hd() 
{
	// !!!!!!经过测试发现，代码里不能出现除以一个2的整数次幂的运算（2除外），否则就会出现invalid opcodeexception
	// 怀疑的编译器BUG，先用>> 代替除法
    MESSAGE msg;
    init_hd();                                          // 先初始化，包括打开硬盘中断等

    while (1)
    {
        // 循环读取消息，主要是接收其他进程的磁盘IO请求消息
        send_recv(RECEIVE, ANY, &msg);

        int src = msg.source;
        switch (msg.type)		// 根据消息的类型来执行相应的硬盘操作
        {
        case DEV_OPEN:
            // hd_identify(msg.DEVICE); //获取硬盘参数
			hd_open(msg.DEVICE);	// 现在需要根据文件系统任务传来的设备号来获取对应硬盘的参数
            break;
        case DEV_CLOSE:
			hd_close(msg.DEVICE);
			break;
		case DEV_READ:
		case DEV_WRITE:
			hd_rdwt(&msg);
			break;
		case DEV_IOCTL:
			hd_ioctl(&msg);
			break;
        default:                            // 收到了消息但是消息类型未定义
			dump_msg("HD driver::unknown msg", &msg);
			spin("FS::main_loop (invalid msg.type)");        
            break;
        }
		// 通知请求IO的进程
		send_recv(SEND, src, &msg);
    }
    
}



/*****************************************************************************
 *                                init_hd
 *****************************************************************************/
/**先初始化硬盘，打开硬盘的硬件中断，指定中断程序
 * <Ring 1> Check hard drive, set IRQ handler, enable IRQ and initialize data
 *          structures.
 *****************************************************************************/
PRIVATE void init_hd()
{
	/* Get the number of drives from the BIOS data area */
	u8 *pNrDrives = (u8*)(0x475);           // 这个地址由BIOS指定，因为BIOS负责硬件检测
	printl("NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);                                 // 学会用assert，防止没有硬盘，那就别玩了。

	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);

	// 初始化硬盘分区信息结构体
	for (int i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
	{
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	}
	hd_info[0].open_cnt = 0;
}




/*****************************************************************************
 *                                hd_open
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_OPEN message. It identify the drive
 * of the given device and read the partition table of the drive if it
 * has not been read.
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PRIVATE void hd_open(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);	/* only one drive */
	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0) {
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
		print_hdinfo(&hd_info[drive]);
	}
}

/*****************************************************************************
 *                                hd_close
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_CLOSE message. 
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PRIVATE void hd_close(int device) {
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);
	hd_info[drive].open_cnt--;
}


/*****************************************************************************
 *                                hd_rdwt
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_READ and DEV_WRITE message.
 * 
 * @param pMsg Message ptr.
 *****************************************************************************/
PRIVATE void hd_rdwt(MESSAGE *pMsg)
{
	int drive = DRV_OF_DEV(pMsg->DEVICE);
	u64 pos = pMsg->POSITION;				// 以字节为单位的目标偏移
	// 扇区号要在int范围内
	assert((pos >> SECTOR_SIZE_SHIFT < (1 << 31)));
	// 只允许从一个扇区的开头开始读取、写入
	// 所以字节偏移的低9位必须全0
	 assert((pos & 0x1FF) == 0);

	u32 sec_nr = (u32) (pos >> SECTOR_SIZE_SHIFT);	// 扇区序号
	int logidx = (pMsg->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;	// 相对于第一个逻辑扇区次设备号的逻辑索引号
	// 扇区号
	sec_nr += pMsg->DEVICE < MAX_PRIM ? 
		hd_info[drive].primary[pMsg->DEVICE].base :
		hd_info[drive].logical[logidx].base;
	
	struct hd_cmd cmd;
	cmd.count = (pMsg->CNT + SECTOR_SIZE) / SECTOR_SIZE;	// 最后一个扇区不足512字节的话最后一个也得读进来
	cmd.features = 0;		// 这个暂时不知道有啥用
	cmd.lba_low = sec_nr & 0xFF;
	cmd.lba_mid = (sec_nr >> 8) & 0xFF;
	cmd.lba_high = (sec_nr >> 16) & 0xFF;
	cmd.device = MAKE_DEVICE_REG(1, drive, (sec_nr >> 24) & 0xF); // 主要用来确定操作模式和主从硬盘
	cmd.command = (pMsg->type == DEV_READ) ? ATA_READ : ATA_WRITE;	// DEV_READ为msg type
	hd_cmd_out(&cmd);

	// 写完控制寄存器后开始读写操作
	int bytes_left = pMsg->CNT;	
	 void *la = (void *)va2la(pMsg->PROC_NR, pMsg->BUF);
	 
	 while (bytes_left > 0)
	 {
		 // 每次读的字节数，除了最后一个扇区，其他都是一个扇区的大小
		 int bytes = min(SECTOR_SIZE, bytes_left);
		 if(pMsg->type == DEV_READ) {
			 	// 读操作
				 interrupt_wait(); // 先等待接收到中断消息，同步通信，所以每收到就阻塞在这里
				 port_read(REG_DATA, hdbuf, SECTOR_SIZE); // 每次读只能读一整个扇区，但是可以只取要的部分
				 // 是memcpy的宏定义，说明代码里指针的地址指的是线性地址，从全局的缓冲区
				 phys_copy(la, (void*)va2la(TASK_HD, hdbuf), bytes); 
		 }
		 else
		 {
			 // 如果HD_TIMEOUT的时间内状态寄存器的相应位都不符合要求，就报错
			 if(!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
				 panic("hd writing error.");
			 }
			 port_write(REG_DATA, la, bytes);	// 向硬盘写数据的时候数据大小可以任意
			 interrupt_wait();		// 写完也要等待操作完毕的中断
		 }
		 // ！如果前面bytes_laft已经小于一个扇区的大小，那这里就会<0导致下一次循环不终止
		 bytes_left -= SECTOR_SIZE;
		 la += SECTOR_SIZE;
	 }
	 
}

/*****************************************************************************
 *                hd_ioctl: 只支持一种消息类型，用来返回请求分区的起始扇区和扇区数
 *****************************************************************************/
/**
 * <Ring 1> This routine handles the DEV_IOCTL message.
 * 
 * @param pMsg  Ptr to the MESSAGE.
 *****************************************************************************/
PRIVATE void hd_ioctl(MESSAGE *pMsg) 
{
	int device = pMsg->DEVICE;
	int drive = DRV_OF_DEV(device);

	struct hd_info *hdi = &hd_info[drive];		// 取当前硬盘信息结构体的指针，方便处理

	if (pMsg->REQUEST == DIOCTL_GET_DEV_INFO)
	{
		// 把请求分区的part_info copy到调用进程的缓冲区
		void *dst = va2la(pMsg->PROC_NR, pMsg->BUF);
		void *src = va2la(TASK_HD, device < MAX_PRIM ? 
			&hdi->primary[device] : 
			&hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE]);
		phys_copy(dst, src, sizeof(struct part_info));
	}
	else
	{
		assert(0);
	}
	
}

/*****************************************************************************
 *                                get_part_table
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive.
 * 
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sect_nr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

/*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 * 
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
PRIVATE void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info * hdi = &hd_info[drive];

	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY) {
		get_part_table(drive, drive, part_tbl);

		int nr_prim_parts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++) { /* 0~3 */
			if (part_tbl[i].sys_id == NO_PART) 
				continue;

			nr_prim_parts++;
			int dev_nr = i + 1;		  /* 1~4 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;

			if (part_tbl[i].sys_id == EXT_PART) /* extended */
				partition(device + dev_nr, P_EXTENDED); // 嵌套读取逻辑分区表
		}
		assert(nr_prim_parts != 0);
	}
	else if (style == P_EXTENDED) {
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++) {
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */

			get_part_table(drive, s, part_tbl);

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

			s = ext_start_sect + part_tbl[1].start_sect;

			/* no more logical partitions
			   in this extended partition */
			if (part_tbl[1].sys_id == NO_PART)
				break;
		}
	}
	else {
		assert(0);
	}
}

/*****************************************************************************
 *                                print_hdinfo
 *****************************************************************************/
/**
 * <Ring 1> Print disk info.
 * 
 * @param hdi  Ptr to struct hd_info.
 *****************************************************************************/
PRIVATE void print_hdinfo(struct hd_info * hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++) {
		printl("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i == 0 ? " " : "     ",
		       i,
		       hdi->primary[i].base,
		       hdi->primary[i].base,
		       hdi->primary[i].size,
		       hdi->primary[i].size);
	}
	for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
		if (hdi->logical[i].size == 0)
			continue;
		printl("         "
		       "%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i,
		       hdi->logical[i].base,
		       hdi->logical[i].base,
		       hdi->logical[i].size,
		       hdi->logical[i].size);
	}
}





/*****************************************************************************
 *                                hd_identify:对应获取硬盘状态的请求消息
 *****************************************************************************/
/**
 * <Ring 1> Get the disk information.
 * 
 * @param drive  Drive Nr.
 *****************************************************************************/
PRIVATE void hd_identify(int drive)
{
	// 需要向相应端口写控制字、命令字
	// 将命令字按照结构写一个结构体
	struct hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;		// 获取硬盘信息的命令
	hd_cmd_out(&cmd);
	interrupt_wait();														// 调用send_recv()等待中断发生后，磁盘中断程序发来的中断消息
																							// 接收到说明可以将数据从磁盘寄存器读进缓冲区了，若未收到，会阻塞
																							// 直到硬盘中断程序调用inform_int通知硬盘中断发生唤醒硬盘驱动任务
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);		// 把磁盘的数据读进hdbuf缓冲区

	print_identify_info((u16*)hdbuf);			// 读到的数据（磁盘信息）显示出来

	// 在缓冲区里填充
	u16* hdinfo = (u16*)hdbuf;

	hd_info[drive].primary[0].base = 0;
	/* Total Nr of User Addressable Sectors */
	hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];

}


/*****************************************************************************
 *                                hd_cmd_out：向硬盘控制器写命令
 *****************************************************************************/
/**
 * <Ring 1> Output a command to HD controller.
 * 
 * @param cmd  The command struct ptr.
 *****************************************************************************/
PRIVATE void hd_cmd_out(struct hd_cmd* cmd)
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 * waitfor等待HD_TIMEOUT的时间并在等待过程中循环读取硬盘状态
	 * 如果这段时间里硬盘都是忙BSY =0，则报错。目的就是等待硬盘的特定状态
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("hd error.");
	// 确定硬盘不忙后，再开始写命令，提自己的想需求
	/* Activate the Interrupt Enable (nIEN) bit */
	out_byte(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	// 依次写寄存器，一旦写完command寄存器，命令就被执行了
	// cmd是结构体变量，包含了要写入寄存器的各个值
	out_byte(REG_CMD,     cmd->command);
}


/*****************************************************************************
 *                                interrupt_wait：硬盘驱动任务等待硬盘中断消息
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 * 
 *****************************************************************************/
PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	send_recv(RECEIVE, INTERRUPT, &msg);
}

/*****************************************************************************
 *                                waitfor：用以确保写硬盘命令之前硬盘状态正常
 *****************************************************************************/
/**
 * <Ring 1> Wait for a certain status.
 * 
 * @param mask    Status mask.
 * @param val     Required status.
 * @param timeout Timeout in milliseconds.
 * 
 * @return One if sucess, zero if timeout.
 *****************************************************************************/
PRIVATE int waitfor(int mask, int val, int timeout)
{
	int t = get_ticks();

	while(((get_ticks() - t) * 1000 / HZ) < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;

	return 0;
}

/*****************************************************************************
 *                            print_identify_info：打印从硬盘读取的硬盘参数
 *****************************************************************************/
/**
 * <Ring 1> Print the hdinfo retrieved via ATA_IDENTIFY command.
 * 
 * @param hdinfo  The buffer read from the disk i/o port.
 *****************************************************************************/
PRIVATE void print_identify_info(u16* hdinfo)
{
	int i, k;
	char s[64];

	// 说明可以在函数里定义局部结构体
	// 这个结构体只在这个函数里用到
	struct iden_info_ascii {
		int idx;
		int len;
		char * desc;
	} iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
		     {27, 40, "HD Model"} /* Model number in ASCII */ };

	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++) {
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		printl("%s: %s\n", iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	printl("LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

	int cmd_set_supported = hdinfo[83];
	printl("LBA48 supported: %s\n",
	       (cmd_set_supported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printl("HD size: %dMB\n", sectors * 512 / 1000000);
}


/*****************************************************************************
 *                                hd_handler：读取硬盘状态，如果已经准备好要的数据，
 * 									则发消息通知硬盘驱动
 *****************************************************************************/
/**
 * <Ring 0> Interrupt handler.
 * 
 * @param irq  IRQ nr of the disk interrupt.
 *****************************************************************************/
PUBLIC void hd_handler(int irq)
{
	/*
	 * Interrupts are cleared when the host
	 *   - reads the Status Register,
	 *   - issues a reset, or
	 *   - writes to the Command Register.
	 */
	hd_status = in_byte(REG_STATUS);

	// 因为不是特定的中断使用，根据各种类型的中断唤醒对应的因为等待该中断而阻塞的进程
	// 所以放在proc.c文件中
	inform_int(TASK_HD);			// 通知进程某个中断发生了（这里是具体的硬盘驱动任务）
	// “通知”就是将对应进程从阻塞转成就绪态，是通过将进程对应的进程表项里的p_flags置1实现的
	
}

