/**************************************************************
 * tty.c
 * 程序功能：终端任务，读取键盘输入缓冲区、显示字符等
 * 修改日期：2019.12.6
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include  "proc.h"
#include "global.h"
#include "keyboard.h"

/*======================================================================*
                           task_tty:tty任务
 *======================================================================*/
PUBLIC void task_tty() 
{
    while (1)
    {
        keyboard_read();
    }
    
}

