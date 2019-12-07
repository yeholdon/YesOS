/**************************************************************
 * keyboard.c
 * 程序功能：键盘IO有关操作
 * 修改日期：2019.12.6
 */
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include  "proc.h"
#include "global.h"
#include "keyboard.h"
#include "keymap.h"

// 键盘缓冲区实例，静态全局变量，只能在本文件被访问
PRIVATE KB_INPUT_BUF    kb_in;          // 需要初始化，在init_keboard()里

PRIVATE	int	code_with_E0 = 0;
PRIVATE	int	shift_l;	/* l shift state */
PRIVATE	int	shift_r;	/* r shift state */
PRIVATE	int	alt_l;		/* l alt state	 */
PRIVATE	int	alt_r;		/* r left state	 */
PRIVATE	int	ctrl_l;		/* l ctrl state	 */
PRIVATE	int	ctrl_r;		/* l ctrl state	 */
PRIVATE	int	caps_lock;	/* Caps Lock	 */
PRIVATE	int	num_lock;	/* Num Lock	 */
PRIVATE	int	scroll_lock;	/* Scroll Lock	 */
PRIVATE	int	column;



/*======================================================================*
                           keyboard_handler:键盘中断处理函数
 *======================================================================*/
PUBLIC  void    keyboard_handler(int    irq) 
{
    // disp_str("*");
    u8 scan_code = in_byte(KB_DATA);

    if (kb_in.count < KB_IN_BYTES) { // 没满
        *(kb_in.p_head) = scan_code;
        kb_in.p_head++;     //先加，如过到头了，后面再调整
        if(kb_in.p_head == kb_in.buf + KB_IN_BYTES) {
            kb_in.p_head = kb_in.buf;
        }
        kb_in.count++;
    }
    // disp_int(scan_code);
}

/*======================================================================*
                           init_keyboard:键盘中断初始化，填入处理函数指针数组
 *======================================================================*/
PUBLIC  void init_keyboard()
{
    kb_in.count = 0;
    kb_in.p_head = kb_in.p_tail = kb_in.buf;

    put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
    enable_irq(KEYBOARD_IRQ);       //默认都关闭，记得打开
}


/*======================================================================*
                           keyboard_read:键盘输入缓冲区读取函数
 *======================================================================*/
PUBLIC  void keyboard_read() 
{
    u8 scan_code;
    int make;           // TRUE: make ; FALSE :break
    char    output[2] = {0, 0};     // 考虑到0xE0或0xE1时扫描码可能为两字节

	u32	key = 0;/* 用一个整型来表示一个键。比如，如果 Home 被按下，
			 * 则 key 值将为定义在 keyboard.h 中的 'HOME'。
			 */
	u32*	keyrow;	/* 指向 keymap[] 的某一行 */

    if (kb_in.count > 0)
    {
        // 操作缓冲区前先关闭中断
        disable_int();          //定义在kliba.asm里
        scan_code = *(kb_in.p_tail);
        kb_in.p_tail++;
        if (kb_in.p_tail == kb_in.buf + KB_IN_BYTES)
        {
            kb_in.p_tail = kb_in.buf;
        }
        kb_in.count--;

        enable_int(); //处理完缓冲区，恢复中断

        // disp_int(scan_code);        //暂时打印一下，看是否调用成功
        // 下面开始解析扫描码
        if (scan_code == 0xE1)
        {
            // 暂时空着
        }
        else if (scan_code == 0xE0)
        {
            // 
            
        }
        else
        {
            // 处理可打印字符
            // 先是无需组合键的小写字符
            
			// 首先判断Make Code 还是 Break Code 
			make = (scan_code & FLAG_BREAK ? 0 : 1);

			// 先定位到 keymap 中的行 
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];
			
			column = 0;
			if (shift_l || shift_r) {
				column = 1;
			}
			if (code_with_E0) {
				column = 2; 
				code_with_E0 = 0;
			}
			
			key = keyrow[column];
			
			switch(key) {
			case SHIFT_L:
				shift_l = make;
				key = 0;
				break;
			case SHIFT_R:
				shift_r = make;
				key = 0;
				break;
			case CTRL_L:
				ctrl_l = make;
				key = 0;
				break;
			case CTRL_R:
				ctrl_r = make;
				key = 0;
				break;
			case ALT_L:
				alt_l = make;
				key = 0;
				break;
			case ALT_R:
				alt_l = make;
				key = 0;
				break;
			default:
				if (!make) {	// 如果是 Break Code , 忽略它
					key = 0;	
				}
				break;
			}

			// 如果 Key 不为0说明是可打印字符，否则不做处理 
			if(key){
				output[0] = key;
				disp_str(output);
            }
            
        }
        
        
        
    }
    
}