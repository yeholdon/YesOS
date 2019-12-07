/**************************************************************
 * console.c
 * 程序功能：控制台和屏幕显示有关的函数
 * 修改日期：2019.12.7
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

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
/*======================================================================*
                         select_console:根据参数指定的console切换控制台
 *======================================================================*/
PUBLIC  void select_console(int nr_console)
{
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES) )
    {
        // console号不合法，直接返回
        return;
    }

    nr_current_console = nr_console;     // 全局变量更新

    set_cursor(console_table[nr_console].cursor);
    set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
                          out_char:从tty缓冲区取出键值，用out_char()显示在对应console中
 *======================================================================*/
PUBLIC void out_char(CONSOLE *p_con, char ch)
{
    // V_MEM_BASE在const.h中定义为0xB8000即显存的起始地址，因为现在只有一个console，所以就是直接的加上disp_pos
    // 后面如果有多个，就要修改了
    u8  *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
    *p_vmem++ = ch;
    *p_vmem++ = DEFAULT_CHAR_COLOR;
    p_con->cursor++;
    set_cursor(p_con->cursor );
}


/*======================================================================*
			    set_cursor：设置光标位置
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}


/*======================================================================*
                          is_current_console:判断当前console是否为当前轮询到的tty对应的console
 *======================================================================*/
PUBLIC int  is_curent_console(CONSOLE *p_con)
{
    return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
                          init_screen:初始化屏幕，也就是初始化CONSOLE变量
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
    int nr_tty = p_tty - tty_table;     // 当前tty的索引序号
    p_tty->p_console = console_table + nr_tty;  //原来init_tty里设定对应console的操作也移到了这里

    int v_mem_size = V_MEM_SIZE >> 1;               // 显存总大小，以Word为单位

    int con_v_mem_size = v_mem_size / NR_CONSOLES;      // 每个console的大小
    p_tty->p_console->v_mem_limit = con_v_mem_size;
    p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
    p_tty->p_console->current_start_addr = p_tty->p_console->original_addr; // 初始就在Console的开头
    p_tty->p_console->cursor = p_tty->p_console->original_addr;     // 默认光标开始处也是开头

    if (nr_tty == 0)
    {
        // 第一个控制台也就是前面一直用的从显存开头其实的那个，就沿用原来光标的位置
        p_tty->p_console->cursor = disp_pos/2;
        disp_pos = 0;       // 这个后面就没用了
    }
    else 
    {
        char prompt[10] = "Ye's OS-";
        for (int i = 0; i < 8; i++)
        {
            out_char(p_tty->p_console, prompt[i]);
        }
        
        out_char(p_tty->p_console, nr_tty + '0'); // 显示一个console序号
        out_char(p_tty->p_console, '$');                    // 再显示一个命令提示符
    }
    set_cursor(p_tty->p_console->cursor);           // 光标也初始化
}