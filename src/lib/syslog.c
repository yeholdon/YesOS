/**************************************************************
 * syslog.c
 * 程序功能：进行文件系统日志的具体操作
 *  lib/syslog.c
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
 *                                syslog
 *****************************************************************************/
/**
 * Write log directly to the disk by sending message to FS.
 * 
 * @param fmt The format string.
 * 
 * @return How many chars have been printed.
 *****************************************************************************/
PUBLIC int syslog(const char *fmt, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /**
						     * 4: size of `fmt' in
						     *    the stack
						     */
	i = vsprintf(buf, fmt, arg);
	assert(strlen(buf) == i);

	if (getpid() == TASK_FS) { /* in FS */
		return disklog(buf);
	}
	else {			/* any proc which is not FS */
		MESSAGE msg;
		msg.type = DISK_LOG;
		msg.BUF= buf;
		msg.CNT = i;
		send_recv(BOTH, TASK_FS, &msg);
		if (i != msg.CNT) {
			panic("failed to write log");
		}

		return msg.RETVAL;
	}
}

