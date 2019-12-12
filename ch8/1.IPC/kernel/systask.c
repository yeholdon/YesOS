/**************************************************************
 * systask.c
 * 程序功能：系统进程
 * 修改日期：2019.12.12
 */

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
 *                                task_sys
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK SYS.
 * 
 *****************************************************************************/
PUBLIC void task_sys()
{
    MESSAGE msg;
    while (1)
    {
        // 循环收取用户进程请求ticks的消息
        send_recv(RECEIVE, ANY, &msg);
        int src_proc = msg.source;

        switch (msg.type)
        {
        case GET_TICKS:
            msg.RETVAL = ticks;
            send_recv(SEND, src_proc, &msg);
            break;
        default:
            panic("unknown msg type");
            break;
        }
    }
    
}