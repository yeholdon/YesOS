/**************************************************************
 * main.c
 * 程序功能：新建了一个文件夹fs用来放文件系统模块，因为文件系统是很重要的一部分
 * 修改日期：2019.12.18
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
// #include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

#include "hd.h"


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

	/* open the device: hard disk */
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
    // 发请求读硬盘数据消息，并等待直到收到可读数据的消息
	send_recv(BOTH, TASK_HD, &driver_msg);

	spin("FS");
}
