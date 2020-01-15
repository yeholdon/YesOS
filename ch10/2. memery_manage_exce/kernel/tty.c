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
#include "tty.h"
#include "console.h"
#include "stdio.h"
#include "string.h"

#define TTY_FIRST   (tty_table)
#define TTY_END     (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY    *p_tty) ;
PRIVATE void	tty_dev_read	(TTY* tty);
PRIVATE void	tty_dev_write	(TTY* tty);
PRIVATE void	tty_do_read	(TTY* tty, MESSAGE* msg);
PRIVATE void	tty_do_write(TTY* tty, MESSAGE* msg);
PRIVATE void put_key(TTY *p_tty, u32 key) ;
/*======================================================================*
                           task_tty:tty任务
 *======================================================================*/
PUBLIC void task_tty() 
{
    // 下面开始扩充task_tty
    TTY *p_tty;
    // 新增IPC操作，将TTY纳入文件系统
    MESSAGE msg;

    init_keyboard();                // 因为键盘也应该被看做TTY的一部分，所以初始化也被放到了这里

    // 初始化所有tty
    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
    {
        init_tty(p_tty);
    }
    
    // nr_current_console = 0; //默认当前console为0号
    select_console(0);
    // 轮询每个tty
    while (1) {
		for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
			do {
				tty_dev_read(p_tty);
				tty_dev_write(p_tty);
			} while (p_tty->inbuf_count);
		}
        
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		assert(src != TASK_TTY);

		TTY* ptty = &tty_table[msg.DEVICE];

		switch (msg.type) {
		case DEV_OPEN:
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;
		case DEV_READ:
			tty_do_read(ptty, &msg);
			break;
		case DEV_WRITE:
			tty_do_write(ptty, &msg);
			break;
		case HARD_INT:
			/**
			 * waked up by clock_handler -- a key was just pressed
			 * @see clock_handler() inform_int()
			 */
			key_pressed = 0;
			continue;
		default:
			dump_msg("TTY::unknown msg", &msg);
			break;
		}
	}

    // while (1)
    // {
    //     for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
    //         tty_dev_read(p_tty);
    //         tty_dev_write(p_tty);
    //     }
    // }
    
}

/*======================================================================*
                          init_tty:初始化tty结构体
 *======================================================================*/
PRIVATE void init_tty(TTY    *p_tty) 
{
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

    init_screen(p_tty);
}



// 原来tty_do_read/write()的功能用新的tty_dev_read/write()来完成
// 由于新的tty要与文件系统关联，所以需要进行IPC。新的tty_do_read/write
// 通过IPC和FS消息通信进行读写的协调，但是不直接进程读写

// tty_dev_read/write 负责从键盘缓冲区读入 字符，并写入进程P的缓冲区（同时回显到控制台）
/*======================================================================*
                          tty_dev_read:从键盘读输入到tty缓冲区
 *======================================================================*/
PRIVATE void tty_dev_read(TTY *p_tty) 
{
    if (is_current_console(p_tty->p_console))
    {
        // 当前console是当前轮询到的TTY对应的console时才读取键盘缓冲区
        // 因为要知道当前的tty，也就是调用该函数的tty，这样才能知道将读到的内容存到哪个tty的缓冲区
        // 所以，keyboard_read()要加一个参数
        keyboard_read(p_tty);
    }
    
}

/*======================================================================*
                          tty_dev_write:从tty缓冲区取出键值，用out_char()显示在对应console中
 *======================================================================*/
PRIVATE void tty_dev_write(TTY *p_tty) 
{
    if (p_tty->inbuf_count != 0)
    {
        char ch = *(p_tty->p_inbuf_tail);
        p_tty->p_inbuf_tail++;
        if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;

        // 进程P通过TTY读入tty_left_cnt的字符
        if (p_tty->tty_left_cnt)
        {
            if (ch >= ' ' && ch <= '~') // 可打印字符，直接输出
            {
                out_char(p_tty->p_console, ch); // 发给进程P的同时也回显
                void *p = p_tty->tty_req_buf + p_tty->tty_trans_cnt;
                // 把把读入的字符复制进进程P的缓冲区
                phys_copy(p, (void *)va2la(TASK_TTY, &ch), 1); 
                p_tty->tty_trans_cnt++;
                p_tty->tty_left_cnt--;
            }
            else if (ch == '\b' && p_tty->tty_trans_cnt > 0)
            {
                // 删除（退格），要保证有东西可删
                out_char(p_tty->p_console, ch);
                p_tty->tty_trans_cnt--;
                p_tty->tty_left_cnt++;
            }
            
            if (ch == '\n' || p_tty->tty_left_cnt == 0)
            {
                out_char(p_tty->p_console, ch);
                // 进程P当次从TTY读入完成，tty发消息解除进程P的挂起
                MESSAGE msg;
                msg.type = RESUME_PROC;
                msg.PROC_NR = p_tty->tty_procnr;
                msg.CNT = p_tty->tty_trans_cnt;
                send_recv(SEND, p_tty->tty_caller, &msg);
                p_tty->tty_left_cnt = 0;
            }
            
        }
        
        // 输出到console
        // out_char(p_tty->p_console, ch);
    }
    
}

// tty_do_read/write负责接收通过FS转发而来的进程P的读TTY请求，
/*****************************************************************************
 *                                tty_do_read：
 *  task_tty接收通过FS转发而来的进程P的读TTY请求DEV_READ，
 *  调用tty_do_read,该函数中，TTY记下发出请求是进程号等信息后立即返回，并回SUSPEND_PROC
 *  消息给进程P(通过FS)，使P保持阻塞，直到收到RESUME_PROC消息。
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_READ message.
 *
 * @note The routine will return immediately after setting some members of
 * TTY struct, telling FS to suspend the proc who wants to read. The real
 * transfer (tty buffer -> proc buffer) is not done here.
 * 
 * @param tty  From which TTY the caller proc wants to read.
 * @param msg  The MESSAGE just received.
 *****************************************************************************/
PRIVATE void tty_do_read(TTY* tty, MESSAGE* msg)
{
	/* tell the tty: */
	tty->tty_caller   = msg->source;  /* who called, usually FS */
	tty->tty_procnr   = msg->PROC_NR; /* who wants the chars */
	tty->tty_req_buf  = va2la(tty->tty_procnr,
				  msg->BUF);/* where the chars should be put */
	tty->tty_left_cnt = msg->CNT; /* how many chars are requested */
	tty->tty_trans_cnt= 0; /* how many chars have been transferred */

	msg->type = SUSPEND_PROC;
	msg->CNT = tty->tty_left_cnt;
	send_recv(SEND, tty->tty_caller, msg);
}


/*****************************************************************************
 *                                tty_do_write:
 * task_tty接收通过FS转发而来的进程P的写TTY请求DEV_WRITE，
 * 调用tty_do_read,该函数中，TTY把P要写入TTY的内容拷贝到TTY的缓冲区中
 * 同时回显到控制台，最后给P回一个SYSCALL_RET的消息表示写TTY完成
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_WRITE message.
 * 
 * @param tty  To which TTY the calller proc is bound.
 * @param msg  The MESSAGE.
 *****************************************************************************/
PRIVATE void tty_do_write(TTY* tty, MESSAGE* msg)
{
	char buf[TTY_OUT_BUF_LEN];
	char * p = (char*)va2la(msg->PROC_NR, msg->BUF);
	int i = msg->CNT;
	int j;

	while (i) {
		int bytes = min(TTY_OUT_BUF_LEN, i);
		phys_copy(va2la(TASK_TTY, buf), (void*)p, bytes);
		for (j = 0; j < bytes; j++)
			out_char(tty->p_console, buf[j]);
		i -= bytes;
		p += bytes;
	}

	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}



/*======================================================================*
                          in_process:处理得到的键盘码
 *======================================================================*/
PUBLIC  void    in_process(TTY *p_tty, u32  key) 
{
    char    output[2] = {'\0', '\0'};
    if (!(key & FLAG_EXT))
    {
        output[0] = key & 0xFF;     // 可打印字符只取8位，至于shift是在取column的时候就考虑进去了，取到的就是对应的加了shift的字符
        // disp_str(output);   
        // 这时就不是在这里直接显示了，而是存入tty的缓冲区
        put_key(p_tty, key);

        // 屏幕显示操作
        // disable_int();
        // out_byte(CRTC_ADDR_REG, CURSOR_H);
        // out_byte(CRTC_DATA_REG, ((disp_pos / 2) >> 8) & 0xFF);
        // out_byte(CRTC_ADDR_REG, CURSOR_L);
        // out_byte(CRTC_DATA_REG, (disp_pos / 2) & 0xFF);
        // enable_int();
    }
    else
    {
        // 功能键中的up/down，shift+up/down来实现滚屏
        int raw_code = key & MASK_RAW; // MASK_RAW = 0x1FFh功能按键的掩码
        switch (raw_code)
        {
        case ENTER:
            put_key(p_tty, '\n');       
            break;
        case BACKSPACE:
            put_key(p_tty, '\b');
            break;
        case UP:
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                scroll_screen(p_tty->p_console, SCR_UP);
            }
            break;
        case DOWN:
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                scroll_screen(p_tty->p_console, SCR_DOWN);
            }
            break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:

		case F6:

		case F7:

		case F8:
		case F9:
            select_console(0);
            break;
		case F10:
            select_console(1);
            break;
		case F11:
            select_console(2);
            break;
		case F12:
            /* Alt + F1~F12 */
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
				select_console(raw_code - F1);
			}
            // out_char(p_tty->p_console, '#');
			break;
        default:
            break;
        }
    }
    
    // 暂时只显示可打印字符以及其和shift的组合字符，其他的非可打印的功能性键暂时还不做出反应
}


/*======================================================================*
                          put_key:往tty缓冲区加控制字符\n等
 *======================================================================*/
PRIVATE void put_key(TTY *p_tty, u32 key) 
{
    if (p_tty->inbuf_count < TTY_IN_BYTES) {
        *(p_tty->p_inbuf_head) = key;
        p_tty->p_inbuf_head++;
        if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_head = p_tty->in_buf;
        }
        p_tty->inbuf_count++;
    }
}


/*======================================================================*
                              tty_write:实现sys_write系统调用的主体功能
*======================================================================*/
PUBLIC  void    tty_write(TTY *p_tty, char *buf, int len)
{
    char *p = buf;
    int i = len;

    while (i)
    {
        // 系统调用的实现就是使用OS的各种函数了
        out_char(p_tty->p_console, *p++);
        i--;
    }
    
}



/*======================================================================*
                              sys_write：write()系统调用的的内核部分
*======================================================================*/
PUBLIC int sys_write(int _unused1, char* buf, int len, PROCESS* p_proc)       // 比write()多一个参数，为了标识调用者进程，将内容输出到进程对应的tty
{
    // 这么做的原因很可能是printf还可能输出到别处，比如文件，所以为了拓展性加了一层调用
    tty_write(&tty_table[p_proc->nr_tty], buf, len);
    return 0;
}



/*======================================================================*
                              sys_printx：支持区分assert来源进程类型的print系统调用
*======================================================================*/
PUBLIC  int sys_printx(int _unused1, int _unuesd2, char *s, PROCESS *p_proc) 
{
    // 这里的前两个参数值得注意，都是unused，其实是因为这里的系统调用最多已经有4个参数了(sys_sendrec)
    // 为了和它统一，加两个没有的参数。
    const   char *p;                // 保证不修改输入的参数字符串
    char ch;
    char reenter_err[] = "?k_reenter id incorrect for unknown reason";  // 第一个字符为了magic char预留
    reenter_err[0] = MAG_CH_PANIC;                                                                    // 根据Magic char是assert还是panic判断是要区分调用系统调用的进程类型做出不同的处理
                                                                                                                                                // 还是直接叫停系统（因为panic值会用在ring0或者ring1）
    /**     作者的注释我也复制过来，方便理解。同时加上自己的中文理解。
	 * @note Code in both Ring 0 and Ring 1~3 may invoke printx().
	 * If this happens in Ring 0, no linear-physical address mapping
	 * is needed.
	 *
	 * @attention The value of `k_reenter' is tricky here. When
	 *   -# printx() is called in Ring 0
	 *      - k_reenter > 0. When code in Ring 0 calls printx(),
	 *        an `interrupt re-enter' will occur (printx() generates
	 *        a software interrupt). Thus `k_reenter' will be increased
	 *        by `kernel.asm::save' and be greater than 0.
	 *   -# printx() is called in Ring 1~3
	 *      - k_reenter == 0.
	 */

    if (k_reenter == 0)
    {
        // 如果不是中断重入，则是正常的进程ring1-3产生的软中断
        p = va2la(proc2pid(p_proc), s); // 虚拟地址转线性地址（加上段基址）
    }
    else if (k_reenter > 0)
    {
        // 如果是在ring0的代码中assert失败产生的中断，一定是中断重入的，因为目前为止内核态都是通过中断进入的
        p = s;          // 内核态的几个段描述符的段基址都是0，所以无需转换了虚拟地址==线性地址
    }
    else
    {
        //正常情况这个不可能发生，除非我们的k_reenter部分代码有错有bug
        p = reenter_err;            
    }


    /** 下面就根据magic char来输出错误提醒
	 * @note if assertion fails in any TASK, the system will be halted;
	 * if it fails in a USER PROC, it'll return like any normal syscall
	 * does.
	 */
    if ((*p == MAG_CH_PANIC) ||                                                       // magic char是panic或者是发生在系统任务的assert，终止系统  
        (*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS]))
    {
        disable_int();
        // 将提示内容直接写到显存，因为是系统出错，要马上停机，显存里的内容不重要了, 而且out_char也不一定还有效
        char *v = (char *) V_MEM_BASE;
        const char *q = p + 1;
        // 而且是将错误位置等循环打印到整个显存空间
        while (v < (char *)(V_MEM_BASE + V_MEM_SIZE))
        {
            *v++ = *q++;
            *v++ = RED_CHAR;        // 用宏MAKE_COLOR来调出别的颜色

            if (!*q)
            {
                // 如果显示的内容显示完了，应该要循环显示
                // 16行一个循环，其余的内容填充
                while (((int)v - V_MEM_BASE) % (16 * SCREEN_WIDTH))
                {
                    // *v++ = ' ';
                    v++;
                    *v++ = GRAY_CHAR;
                }
                q = p + 1;
            }
            
        }
        
        // 显示完毕，停机
        __asm__ __volatile__("hlt");
    }
    
    while ((ch = *p++) != 0)
    {
        if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
        {
            // 其实因为前面已经判断过 了panic，这里的panic可以不用写的
            continue; // 跳过magic char
        }
        out_char(tty_table[p_proc->nr_tty].p_console, ch); //正常在对应的console显示
        
    }
        
    return 0;
}