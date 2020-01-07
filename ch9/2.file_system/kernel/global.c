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
#include "fs.h"

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
PUBLIC	TASK	task_table[NR_TASKS] = { {task_tty, STACK_SIZE_TTY, "tty"},
																					  {task_sys, STACK_SIZE_SYS, "sys"},
																					  {task_hd,  STACK_SIZE_HD,  "HD" },
																					  {task_fs,  STACK_SIZE_FS,  "FS" }, 
																					  };

PUBLIC	irq_handler irq_table[NR_IRQ];		// 声明在global.h中。NR_IRQ=16，以对应主从两个8259A，定义在const.h中

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {sys_get_ticks, 
																											  sys_write, 
																											  sys_sendrec,
																											  sys_printx};  

PUBLIC	TTY	tty_table[NR_CONSOLES]; 
PUBLIC	CONSOLE	console_table[NR_CONSOLES];		// 目前为止，tty和console是一一对应的

/* FS related below 文件系统相关*/
/*****************************************************************************/
/**
 * For dd_map[k],
 * `k' is the device nr.\ dd_map[k].driver_nr is the driver nr.
 *
 * Remeber to modify include/const.h if the order is changed.
 *****************************************************************************/
struct dev_drv_map dev_to_dri_map[] = {
	/* driver nr.		major device nr.
	   ----------		---------------- */
	{INVALID_DRIVER},	/**< 0 : Unused */
	{INVALID_DRIVER},	/**< 1 : Reserved for floppy driver */
	{INVALID_DRIVER},	/**< 2 : Reserved for cdrom driver */
	{TASK_HD},		/**< 3 : Hard disk */
	{TASK_TTY},		/**< 4 : TTY */
	{INVALID_DRIVER}	/**< 5 : Reserved for scsi disk driver */
};

/**
 * 6MB~7MB: buffer for FS
 */
PUBLIC	u8 *		fsbuf		= (u8*)0x600000;
PUBLIC	const int	FSBUF_SIZE	= 0x100000;