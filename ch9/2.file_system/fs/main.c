/**************************************************************
 * main.c
 * 程序功能：新建了一个文件夹fs用来放文件系统模块，因为文件系统是很重要的一部分
 * 修改日期：2019.12.18
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "config.h"

#include "hd.h"

PRIVATE void init_fs();
PRIVATE void mkfs();

/*****************************************************************************
 *                                task_fs:文件系统任务，这里充当那个请求硬盘IO的用户进程的作用
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK FS.
 * 
 *****************************************************************************/
PUBLIC void task_fs()
{
	printl("Task FS begins.\n");

	// /* open the device: hard disk */ 转到init_fs()函数中
	// MESSAGE driver_msg;
	// driver_msg.type = DEV_OPEN;
	// // printl("%d\n", ROOT_DEV);
	// driver_msg.DEVICE = (int) MINOR(ROOT_DEV);			// 启动设备号，由主设备号3也就是代表从硬盘启动，从设备号16（第二个分区的第一个逻辑分区）两部分
	// 																									// 组成，代表从该分区启动这个分区启动
	// // printf("%d", driver_msg.DEVICE);
	// assert(dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
    // // 发请求读硬盘数据消息，并等待直到收到可读数据的消息
	// send_recv(BOTH, dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	spin("FS");
}


/*****************************************************************************
 *                                init_fs
 *****************************************************************************/
/**
 * <Ring 1> Do some preparation.
 * 
 *****************************************************************************/
PRIVATE void init_fs()
{
	/* open the device: hard disk */
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	// printl("%d\n", ROOT_DEV);
	driver_msg.DEVICE = (int) MINOR(ROOT_DEV);			// 启动设备号，由主设备号3也就是代表从硬盘启动，从设备号16（第二个分区的第一个逻辑分区）两部分
																										// 组成，代表从该分区启动这个分区启动
	// printf("%d", driver_msg.DEVICE);
	assert(dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
    // 发请求读硬盘数据消息，并等待直到收到可读数据的消息
	send_recv(BOTH, dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
