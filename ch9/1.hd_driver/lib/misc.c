
/****************************************************************************
 * misc.c
 * 程序功能：放未归类的内容
 * 修改日期：2019.12.10
 misc其实是英文miscellaneous的前四个字母，杂项、混合体、大杂烩的意思
在linux的源码中可以看到与misc相关的文件或函数名，使用misc来命名主要是
表示该文件还没归类好，不知道将它归到哪个方面或者放置在哪个地方比较好，
所以暂时用misc。比如在include\linux\文件夹下，有一个miscdevice.h头文件；
在代码里面也会经常碰到misc前缀的变量名或者函数。
*/

// @param标签提供了对某个函数的参数的各项说明，包括参数名、参数数据类型、描述等。
// 是JSDoc中的一个标签：可以用于快速生成js库的API文档，但是得按照它的规范来写注释

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"


/*****************************************************************************
 *                                spin：自旋，其实就是表示死循环在调用该函数的函数里
 *****************************************************************************/
PUBLIC  void    spin(char *function_name) 
{
    printl("\nspinning in %s...\n", function_name);
    while(1) { };
}


/*****************************************************************************
 *                                assertion_failure：assert失败，打印出错位置后让进程停转
 *****************************************************************************/
PUBLIC  void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printl("%c  assert(%s) failed: file: %s, base_file: %s, ln%d",
	       MAG_CH_ASSERT,
	       exp, file, base_file, line);

	/**
	 * If assertion fails in a TASK, the system will halt before
	 * printl() returns. If it happens in a USER PROC, printl() will
	 * return like a common routine and arrive here. 
	 * @see sys_printx()
	 * 
	 * We use a forever loop to prevent the proc from going on:
	 */
	spin("assertion_failure()");

	/* should never arrive here 那为啥还放这里？？？
    这个是内联汇编，__asm__ 和__volatile__分别是asm和volatile这两个GCC关键字的
    宏定义，后者表示静止编译器优化后面的汇编指令。
     */
        __asm__ __volatile__("ud2");
}