/**************************************************************
 * global.c
 * 程序功能：存放全局变量的定义，保证只有包含在这里的一份是定义
 * 修改日期：2019.11.29
 */

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include  "proc.h"
#include "global.h"
#include "tty.h"
#include "console.h"

PUBLIC  PROCESS proc_table[NR_PROCS + NR_TASKS];		// 调度还是一起调度


PUBLIC  char    task_stack[STACK_SIZE_TOTAL];

// 这个是对应s_task的数组，用来批量初始化proc_table用的
/*
 这只是暂时的，后面应该有专门的方法来新建一个用户进程，而不是像现在用宏定义预先定义好用户进程数
应该要能够动态新建回收用户进程
*/
PUBLIC	TASK	user_proc_table[NR_PROCS] = {		
					{TestA, STACK_SIZE_TESTA, "TestA"}, 
					{TestB, STACK_SIZE_TESTB, "TestB"}, 
					{TestC, STACK_SIZE_TESTC, "TestC"}};
PUBLIC	TASK	task_table[NR_TASKS] = { {task_tty, STACK_SIZE_TTY, "tty"},};

PUBLIC	irq_handler irq_table[NR_IRQ];		// 声明在global.h中。NR_IRQ=16，以对应主从两个8259A，定义在const.h中

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {sys_get_ticks, 
																											  sys_write};  // 只有一个，这里就不用专门的函数给它赋值了

PUBLIC	TTY	tty_table[NR_CONSOLES]; 
PUBLIC	CONSOLE	console_table[NR_CONSOLES];		// 目前为止，tty和console是一一对应的

