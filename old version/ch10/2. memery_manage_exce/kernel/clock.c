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
#include "stdio.h"

/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	// disp_str("#");

    // ticks++;            // 这个是新加的
    // p_proc_ready->ticks--;

    // if (k_reenter != 0) 
    // {
    //     return;
    // }

    // // 新的策略，一个进程运行完它的优先级对应的时间后再换下一个
    // if (p_proc_ready->ticks > 0) 
    // {
    //     return;
    // }

	if (++ticks >= MAX_TICKS)
		ticks = 0;

	if (p_proc_ready->ticks)
		p_proc_ready->ticks--;

	if (key_pressed)
		inform_int(TASK_TTY);

	if (k_reenter != 0) {
		return;
	}

	if (p_proc_ready->ticks > 0) {
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


/*======================================================================*
                           init_clock:初始化时钟中断
 *======================================================================*/
PUBLIC void init_clock()
{
    /* 初始化 8253 PIT */
    out_byte(TIMER_MODE, RATE_GENERATOR);   // 模式2
    out_byte(TIMER0, (u8) (TIMER_FREQ/HZ));         // low byte
    out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));  // high byte

    put_irq_handler(CLOCK_IRQ, clock_handler);   //设定时钟中断处理程序 
	enable_irq(CLOCK_IRQ);										         //让8259A可以接收时钟中断 
}