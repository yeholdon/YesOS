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
	// disp_str("#");

    ticks++;            // 这个是新加的
    p_proc_ready->ticks--;

    if (k_reenter != 0) 
    {
        return;
    }

    // 新的策略，一个进程运行完它的优先级对应的时间后再换下一个
    if (p_proc_ready->ticks > 0) 
    {
        return;
    }

    schedule();
    
}


/*======================================================================*
                              milli_delay: accuracy = 10ms
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
}

