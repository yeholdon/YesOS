/**************************************************************
 * lib/getpid.c
 * 程序功能：获取当前进程ID
 * 修改日期：2020.1.10
 */

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*****************************************************************************
 *                                getpid
 *****************************************************************************/
/**
 * Get the PID.
 * 
 * @return The PID.
 *****************************************************************************/
PUBLIC int getpid()
{
	MESSAGE msg;
	msg.type	= GET_PID;

	send_recv(BOTH, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.PID;
}
