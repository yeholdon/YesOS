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

PRIVATE void    init_hd			();
PRIVATE void	hd_cmd_out		(struct hd_cmd* cmd);
PRIVATE int waitfor	(int mask, int val, int timeout);
PRIVATE void	interrupt_wait		();
PRIVATE	void	hd_identify		(int drive);
PRIVATE void	print_identify_info	(u16* hdinfo);

PRIVATE	u8	hd_status;                                  // 硬盘状态别的函数也要用到，全局
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];   // 两个扇区的缓冲区

/*****************************************************************************
 *                                task_hd
 *****************************************************************************/
/**
 * Main loop of HD driver.硬盘驱动的主循环，初始化后不断循环接收消息
 * 
 *****************************************************************************/
PUBLIC void task_hd() 
{
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
            hd_identify(0); //获取硬盘参数
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

