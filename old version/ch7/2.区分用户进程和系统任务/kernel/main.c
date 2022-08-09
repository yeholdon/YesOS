/**************************************************************
 * main.c
 * 程序功能：一个简单进程体，即一个函循环打印字符的函数
 * 修改日期：2019.12.2
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                            kernel_main:内核主函数
 *======================================================================*/
PUBLIC  int kernel_main() 
{
    disp_str("-----\"kernel_main\" begins-----\n");


    // 下面初始化设置进程表
    TASK *p_task = task_table;
    PROCESS *p_proc = proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;  // 初始位置是栈底
	u16		selector_ldt	= SELECTOR_LDT_FIRST;


	// 要区分用户进程和系统任务的特权级等，所以特权级相关的几项属性要单独设置，而不是统一一样了
	u8	privilege;
	u8	rpl;
	int eflags;
    for(int i=0;i<NR_TASKS + NR_PROCS;i++){		// 这里要分成用户进程和任务

		if (i < NR_TASKS)		//先设置系统任务特权级
		{
			p_task = task_table + i;
			privilege = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202;  // IF = 1, IOPL = 1
		}
		else
		{
			p_task = user_proc_table + (i - NR_TASKS);
			privilege = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags = 0x202;		// IF = 1, IOPL = 0，用户进程没有IO权限
		}
		
		

		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid直接设为循环变量

		p_proc->ldt_sel = selector_ldt;     // idt选择子先设好了，另外还要生成其GDT里的描述符

        // IDT的生成
		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege<< 5;									// 更新
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));	
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;							// 更新
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;																				// 更新
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| rpl;

        // TASK里的是几个关键的每个进程不一样的信息,进程入口地址、堆栈栈顶和进程名
		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;  // 更新

		p_proc->nr_tty = 0;				// 默认进程与tty0绑定，后面再修改

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 15;
	proc_table[1].ticks = proc_table[1].priority =  5;
	proc_table[2].ticks = proc_table[2].priority =  5;
	proc_table[3].ticks = proc_table[3].priority =  5;

	// 初始化后重新赋值进程的tty
	proc_table[1].nr_tty = 0;
	proc_table[2].nr_tty = 1;
	proc_table[3].nr_tty = 1;

	k_reenter = 0;			// 为了统一，现在在第一个进程执行前，也让k_reenter自增了，所以这里k_reenter初值要改一下
    ticks = 0;  
	p_proc_ready	= proc_table; //一个指向下一个要启动进程的进程表的指针，在kernel.asm中导入使用
	

	// 初始化键盘
	init_keyboard();

    // 初始化8253 PIT
	init_clock();

	// disp_pos = 0;
	// for (int i = 0; i < 80*25; i++) {
	// 	disp_str(" ");
	// }
	// disp_pos = 0;

	restart();                                          // kernel.asm中的函数
    while(1) 
    {
        //暂时先死循环
    }
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1) {
		// disp_color_str("A.", BRIGHT | MAKE_COLOR(BLACK, RED));
		// disp_int(get_ticks());
		printf("<Ticks : %x>", get_ticks());
		milli_delay(2000);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while(1){
		// disp_color_str("B.", BRIGHT | MAKE_COLOR(BLACK, RED));
		// disp_int(get_ticks());
		printf("B");
		milli_delay(2000);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while(1){
		// disp_color_str("C.", BRIGHT | MAKE_COLOR(BLACK, RED));
		// disp_int(get_ticks());
		printf("C");
		milli_delay(2000);
	}
}


