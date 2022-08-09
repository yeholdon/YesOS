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

#define TTY_FIRST   (tty_table)
#define TTY_END     (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY    *p_tty) ;
PRIVATE void tty_do_read(TTY *p_tty) ;
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY *p_tty, u32 key) ;
/*======================================================================*
                           task_tty:tty任务
 *======================================================================*/
PUBLIC void task_tty() 
{
    // 下面开始扩充task_tty
    TTY *p_tty;
    init_keyboard();                // 因为键盘也应该被看做TTY的一部分，所以初始化也被放到了这里

    // 初始化所有tty
    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
    {
        init_tty(p_tty);
    }
    
    // nr_current_console = 0; //默认当前console为0号
    select_console(0);
    // 轮询每个tty
    while (1)
    {
        for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
            tty_do_read(p_tty);
            tty_do_write(p_tty);
        }
    }
    

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


/*======================================================================*
                          tty_do_read:从键盘读输入到tty缓冲区
 *======================================================================*/
PRIVATE void tty_do_read(TTY *p_tty) 
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
                          tty_do_write:从tty缓冲区取出键值，用out_char()显示在对应console中
 *======================================================================*/
PRIVATE void tty_do_write(TTY *p_tty) 
{
    if (p_tty->inbuf_count != 0)
    {
        char ch = *(p_tty->p_inbuf_tail);
        p_tty->p_inbuf_tail++;
        if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;

        // 输出到console
        out_char(p_tty->p_console, ch);
    }
    
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