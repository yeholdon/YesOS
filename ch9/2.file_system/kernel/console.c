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
#include "stdio.h"

PRIVATE void clear_screen(int pos, int len);
PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void auto_scroll_screen(CONSOLE *p_con);
PRIVATE void flush(CONSOLE* p_con);
PUBLIC int  is_current_console(CONSOLE *p_con);
PRIVATE	void w_copy(unsigned int dst, const unsigned int src, int size);
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

    flush(&console_table[nr_console]);
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
    // V_MEM_BASE在const.h中定义为0xB8000即显存的起始地址
    u8  *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
    // 自动滚屏到输入光标位置
    auto_scroll_screen(p_con);

	/*
	 * calculate the coordinate of cursor in current console (not in
	 * current screen)
	 */
	int cursor_x = (p_con->cursor - p_con->original_addr) % SCREEN_WIDTH;
	int cursor_y = (p_con->cursor - p_con->original_addr) / SCREEN_WIDTH;

    switch (ch)
    {
    case '\n' :
        if (p_con->cursor < (p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH)) // 没到CONSOLE的最后一行
        {
            p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
                                                                                                ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
            
        }
        break;
    case '\b':
        if (p_con->cursor > p_con->original_addr)
        {
            p_con->cursor--;
            *(p_vmem - 2) = ' ';
            *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
        }
        break;
    default:    // default就是常规字符显示
        if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1)
        {
            *p_vmem++ = ch;
            *p_vmem++ = DEFAULT_CHAR_COLOR;
            p_con->cursor++;
        }
        break;
    }

    // if (p_con->cursor - p_con->original_addr >= p_con->v_mem_limit) {
	// 	cursor_x = (p_con->cursor - p_con->original_addr) % SCREEN_WIDTH;
	// 	cursor_y = (p_con->cursor - p_con->original_addr) / SCREEN_WIDTH;
	// 	int cp_orig = p_con->original_addr + (cursor_y + 1) * SCREEN_WIDTH - SCREEN_SIZE;
	// 	w_copy(p_con->original_addr, cp_orig, SCREEN_SIZE - SCREEN_WIDTH);
	// 	p_con->current_start_addr = p_con->original_addr;
	// 	p_con->cursor = p_con->original_addr + (SCREEN_SIZE - SCREEN_WIDTH) + cursor_x;
	// 	clear_screen(p_con->cursor, SCREEN_WIDTH);
	// 	if (!p_con->is_full)
	// 		p_con->is_full = 1;
	// }

    assert(p_con->cursor - p_con->original_addr < p_con->v_mem_limit);

    auto_scroll_screen(p_con);
    flush(p_con);
}


/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
	if (is_current_console(p_con)) {                // 是当前控制台再刷新显示，将p_con的信息通过写寄存器更新到显卡
		set_cursor(p_con->cursor);
		set_video_start_addr(p_con->current_start_addr);
	}
}

/*======================================================================*
			    auto_scroll_screen(CONSOLE *p_con)：自动滚屏到输入光标位置
 *======================================================================*/
PRIVATE void auto_scroll_screen(CONSOLE *p_con)
{
    if(p_con->current_start_addr + SCREEN_SIZE < p_con->cursor) {
        int rows = (p_con->cursor - (p_con->current_start_addr + SCREEN_SIZE )) / SCREEN_WIDTH + 1; 
        for (int i = 0; i < rows; i++)
        {
            scroll_screen(p_con, SCR_DOWN);
        }
    }

    if (p_con->cursor < p_con->current_start_addr)
    {
        int rows = (p_con->current_start_addr  -  p_con->cursor) / SCREEN_WIDTH + 1; 
        for (int i = 0; i < rows; i++)
        {
            scroll_screen(p_con, SCR_UP);
        }       
    }
    
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

/*****************************************************************************
 *                                clear_screen:从pos位置开始清除len个字符
 *****************************************************************************/
/**
 * Write whitespaces to the screen.
 * 
 * @param pos  Write from here.
 * @param len  How many whitespaces will be written.
 *****************************************************************************/
PRIVATE void clear_screen(int pos, int len)
{
	u8 * pch = (u8*)(V_MEM_BASE + pos * 2);
	while (--len >= 0) {
		*pch++ = ' ';
		*pch++ = DEFAULT_CHAR_COLOR;
	}
}

/*======================================================================*
                          is_current_console:判断当前console是否为当前轮询到的tty对应的console
 *======================================================================*/
PUBLIC int  is_current_console(CONSOLE *p_con)
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
        char prompt[20] = "Ye's OS - Console";
        for (int i = 0; i < 17; i++)
        {
            out_char(p_tty->p_console, prompt[i]);
        }
        
        out_char(p_tty->p_console, nr_tty + '0'); // 显示一个console序号
        out_char(p_tty->p_console, ' ');  
        out_char(p_tty->p_console, '$');                    // 再显示一个命令提示符
        out_char(p_tty->p_console, ' ');                       // 提示符后加一个空格，好看点
    }
    set_cursor(p_tty->p_console->cursor);           // 光标也初始化
}


/*======================================================================*
                          scroll_screen:滚动屏幕，按一下，滚动一行，直到到达边界
 *======================================================================*/
PUBLIC  void    scroll_screen(CONSOLE *p_con, int direction)
{
    if (direction == SCR_UP)
    {
        if (p_con->current_start_addr > p_con->original_addr)
        {
            p_con->current_start_addr -= SCREEN_WIDTH;
        }
        
    }
    else if (direction == SCR_DOWN)
    {
        if (p_con->current_start_addr + SCREEN_SIZE < p_con->original_addr + p_con->v_mem_limit)
        {
            p_con->current_start_addr += SCREEN_WIDTH;
        }
        
    }
    else
    {
        // 其他按键，没有反应
    }

    // CONSOLE结构里的项都要用set函数写入相应寄存器才能生效
    // set_video_start_addr(p_con->current_start_addr);
    // set_cursor(p_con->cursor);                                                  // 不要忘了光标
    flush(p_con);
}

/*****************************************************************************
 *                                w_copy:以字为单位 在内存中拷贝
 *****************************************************************************/
/**
 * Copy data in WORDS.
 *
 * Note that the addresses of dst and src are not pointers, but integers, 'coz
 * in most cases we pass integers into it as parameters.
 * 
 * @param dst   Addr of destination.
 * @param src   Addr of source.
 * @param size  How many words will be copied.
 *****************************************************************************/
PRIVATE	void w_copy(unsigned int dst, const unsigned int src, int size)
{
	phys_copy((void*)(V_MEM_BASE + (dst << 1)),
		  (void*)(V_MEM_BASE + (src << 1)),
		  size << 1);
}