/**************************************************************
 * tty.h
 * 程序功能：tty结构定义
 * 修改日期：2019.12.7
 */

#ifndef _YES_TTY_H_
#define _YES_TTY_H_

#define TTY_IN_BYTES	256	/* tty input queue size */
#define TTY_OUT_BUF_LEN		2	/* tty output buffer size */

struct s_console;      // 用到的控制台结构的前置声明

typedef struct s_tty
{
	u32	in_buf[TTY_IN_BYTES];	/* TTY 输入缓冲区 */
	u32*	p_inbuf_head;		        /* 指向缓冲区中下一个空闲位置 */
	u32*	p_inbuf_tail;		            /* 指向键盘任务应处理的键值 */
	int	inbuf_count;		                  /* 缓冲区中已经填充了多少 */
	// 将TTY纳入文件系统统一管理，为IPC新增
	int	tty_caller;		// 向TTY发消息的进程，通常为FS
	int	tty_procnr;		// 请求数据的进程P
	void*	tty_req_buf;	// 进程P用来存放读入字符的缓冲区线性地址
	int	tty_left_cnt;			// P想要读入的字符数
	int	tty_trans_cnt;		// TTY已经向P传送的字符数

	struct s_console *	p_console;      // 指向对应console的指针
} TTY;


#endif