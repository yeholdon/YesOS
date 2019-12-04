/**************************************************************
 * clock.c
 * 程序功能：存放时钟中断中，进程调度，获取目标进程的函数
 * 修改日期：2019.11.29
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"


/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	disp_str("#");
    p_proc_ready++;
    if(p_proc_ready >= proc_table + NR_TASKS)
    {
        p_proc_ready = proc_table;
    }
}
