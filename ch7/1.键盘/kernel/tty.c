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
    if (is_curent_console(p_tty->p_console))
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
        if (p_tty->inbuf_count < TTY_IN_BYTES) {
            *(p_tty->p_inbuf_head) = key;
            p_tty->p_inbuf_head++;
            if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
                p_tty->p_inbuf_head = p_tty->in_buf;
            }
            p_tty->inbuf_count++;
        }
        
        

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
        case UP:
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                disable_int();
                out_byte(CRTC_ADDR_REG, START_ADDR_H);
                out_byte(CRTC_DATA_REG, ((80*15) >> 8) & 0xFF); // 滚半屏
                out_byte(CRTC_ADDR_REG, START_ADDR_L);
                out_byte(CRTC_DATA_REG, ((80*15) & 0xFF));
                enable_int();
            }
            break;
        case DOWN:
            if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                // shift +down暂时啥也不做
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
		case F10:
		case F11:
		case F12:
            /* Alt + F1~F12 */
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
				select_console(raw_code - F1);
			}
            out_char(p_tty->p_console, '#');
			break;
        default:
            break;
        }
    }
    
    // 暂时只显示可打印字符以及其和shift的组合字符，其他的非可打印的功能性键暂时还不做出反应
}