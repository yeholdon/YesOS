/*****************************************************
 * proc.c
 * 程序功能：进程相关的函数，包括系统调用函数
 * 修改日期：2019.12.5
 */
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include  "proc.h"
#include "global.h"


PUBLIC int sys_get_ticks() 
{
    // disp_str("+");          // 先随便输出一个字符来看看是否调用到
    return ticks;
}


// 调度算法
PUBLIC  void    schedule() 
{
    PROCESS *p;
    int     greatest_ticks = 0;

    while (!greatest_ticks)
    {
        // 把ticks最大的进程设为下一个被调度的进程
        for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++)     // 分成task和process
        {
            if (p->ticks > greatest_ticks)
            {
                // disp_str("<");
				// disp_int(p->ticks);
				// disp_str(">");
                greatest_ticks = p->ticks;
                p_proc_ready = p;
            }
            
        }

        // 如果所有进程ticks都是0，就重新初始化为priority值
        if (!greatest_ticks)
        {
            for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) // 分成task和process
            {
                p->ticks = p->priority;
            }
        }
        
    }
    
}