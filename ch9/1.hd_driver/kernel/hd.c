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
#include "keymap.h"
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
        // 循环读取消息
        send_recv(RECEIVE, ANY, &msg);

        int src = msg.source;
        switch (msg.type)
        {
        case DEV_OPEN:
            hd_identity(0); //获取硬盘参数
            break;
        
        default:                            // 收到了消息但是消息类型未定义
			dump_msg("HD driver::unknown msg", &msg);
			spin("FS::main_loop (invalid msg.type)");        
            break;
        }
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
 *                                hd_identify:获取硬盘状态
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
	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	print_identify_info((u16*)hdbuf);
}



/*****************************************************************************
 *                                hd_handler
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

	inform_int(TASK_HD);
}

