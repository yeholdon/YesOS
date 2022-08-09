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