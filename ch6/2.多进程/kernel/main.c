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
    PROCESS *p_proc = proc_table;

    p_proc->ldt_sel = SELECTOR_LDT_FIRST;
    // 方便起见，可以从GDT复制后修改
    memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR)); // 选择子高13位才是索引第3位是TI和RPL
    p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK  << 5;        //改DPL为1
    memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
    p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;

    // 第一个数字其实就是表示改描述符在LDT中的索引，因为低3位不是索引，所以第1项应该是1<<3
    // TI区别GDT(1)和LDT(0)选择子
    p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
	p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
    p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;    //GS指向显存段
	p_proc->regs.eip= (u32)TestA;
	p_proc->regs.esp= (u32) task_stack + STACK_SIZE_TOTAL;
	p_proc->regs.eflags = 0x1202;	// IF=1, IOPL=1, bit 2 is always 1.

	p_proc_ready	= proc_table; //一个指向下一个要启动进程的进程表的指针，在kernel.asm中导入使用
	restart();                                          // kernel.asm中的函数

    while(1) 
    {
        //暂时先死循环
    }
}

void TestA() 
{
    int i = 0;
    while (1)
    {
        disp_str("A");
        disp_int(i++);
        disp_str(".");
        delay(1);
    }
    
}