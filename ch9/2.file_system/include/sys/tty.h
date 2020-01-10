/**************************************************************
 * tty.h
 * 程序功能：tty结构定义
 * 修改日期：2019.12.7
 */

#ifndef _YES_TTY_H_
#define _YES_TTY_H_

#define TTY_IN_BYTES	256	/* tty input queue size */

struct s_console;      // 用到的控制台结构的前置声明

typedef struct s_tty
{
	u32	in_buf[TTY_IN_BYTES];	/* TTY 输入缓冲区 */
	u32*	p_inbuf_head;		        /* 指向缓冲区中下一个空闲位置 */
	u32*	p_inbuf_tail;		            /* 指向键盘任务应处理的键值 */
	int	inbuf_count;		                  /* 缓冲区中已经填充了多少 */

	struct s_console *	p_console;      // 指向对应console的指针
} TTY;


#endif