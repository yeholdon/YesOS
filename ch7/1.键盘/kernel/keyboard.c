/**************************************************************
 * keyboard.c
 * 程序功能：键盘IO有关操作
 * 修改日期：2019.12.6
 */
#include "type.h"
#include "const.h"
#include "protect.h"
#include "console.h"
#include "tty.h"
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

PRIVATE u8  get_byte_from_kbbuf();

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
PUBLIC  void keyboard_read(TTY *p_tty) 
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
        code_with_E0 = 0;

        scan_code = get_byte_from_kbbuf();

        // disp_int(scan_code);        //暂时打印一下，看是否调用成功
        // 下面开始解析扫描码
        if (scan_code == 0xE1)
        {
            // 以0xE1 开头的只有Pause键，一个键共6个码
            u8 pause_break_code[] = {0xE1, 0x1D, 0x45, 
                                                                    0xE1, 0x9D, 0xC5};
            int is_pause_break = 1;
            // 下面开始循环读取接下来的5个码，一旦有一个不符合就说明不匹配
            for (int i = 1; i < 6; i++)
            {
                if (get_byte_from_kbbuf() != pause_break_code[i])
                {
                    is_pause_break = 0;
                    break;
                }
                
            }
            
        }
        else if (scan_code == 0xE0)
        {
            // 0xE0开头的只有Print screen键是make和break都是4个码，特殊判断
            scan_code = get_byte_from_kbbuf();

            // PrintScreen被按下, Make码
            if (scan_code == 0xB7)
            {
                if (get_byte_from_kbbuf() == 0xE0)
                {
                    if (get_byte_from_kbbuf() == 0x37)
                    {
                        key = PRINTSCREEN; //连着4个码都符合，才确定是PrintScreen
                        make = TRUE;                // make 码
                    }
                    
                }
                
            }

            // PrintScreen被释放，Break码
            if (scan_code == 0xB7)
            {
                if (get_byte_from_kbbuf() == 0xE0)
                {
                    if (get_byte_from_kbbuf() == 0xAA)
                    {
                        key = PRINTSCREEN;
                        make = FALSE;
                    }
                    
                }
                
            }
            
            // 除去PrintScreen的情况，剩下的0xE0开头的码都是make和break各两个，可以统一处理
            if (key == 0)
            {
                // 通过全局变量来标识，说明是非PrintScreen和shift的非可打印字符
                code_with_E0 = 1;
            }
            
            
        }
        
        if ((key != PAUSEBREAK) && (key != PRINTSCREEN))
        {
            // 处理可打印字符
            // 先是无需组合键的小写字符，组合键的情况还是要多次调用keyboard_read()，也没啥关系
            
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
				break;
			case SHIFT_R:
				shift_r = make;
				break;
			case CTRL_L:
				ctrl_l = make;
				break;
			case CTRL_R:
				ctrl_r = make;
				break;
			case ALT_L:
				alt_l = make;
				break;
			case ALT_R:
				alt_l = make;
				break;
			default:
                /* 放到后面统一处理 */
				// if (!make) {	// 如果是 Break Code , 忽略它。
				// 	key = 0;	
				// }
				break;
			}

            if (make)       // Break码忽略
            {
                // 因为非可打印字符得到的码不是ascci码，而key是32位的，ascii码只有8位，
                // 所以其他位可以用来标识剩下的非可打印字符，这里对于非可打印字符统一用宏FLAG_EXT进一步标识
                // 剩下的shift、ctrl等键的状态，则是直接通过或操作，添加到常规字符key中除了8位ASCII码和1位FLAG_EXT外的那些位上。
                // 加shift的这种组合键还是要调用两次keyboard_read()来完成的，毕竟按了两个键
                key |= shift_l ? FLAG_SHIFT_L : 0;
                key |= shift_r ? FLAG_SHIFT_R : 0;
                key |= ctrl_l	? FLAG_CTRL_L	: 0;
				key |= ctrl_r	? FLAG_CTRL_R	: 0;
				key |= alt_l	? FLAG_ALT_L	: 0;
				key |= alt_r	? FLAG_ALT_R	: 0;

                // 完成后，可打印字符和非可打印字符统一用一个in_process函数统一处理
                in_process(p_tty, key);         // in_process也得加标识当前tty的参数
            }
            
            
        }
        
        
        
    }
    
}


/*======================================================================*
                           get_byte_from_kbbuf:从键盘输入缓冲区读取一个字符
 *======================================================================*/
PRIVATE u8 get_byte_from_kbbuf() 
{

    u8 scan_code;
    // 调用时可能缓冲区为空，因为这个部分用单独了任务实现了，所以可以循环等待字符的输入
    while (kb_in.count <= 0)
    {
    }
    
    // 缓冲区非空
    disable_int();

    scan_code = *(kb_in.p_tail);
    kb_in.p_tail++;
    if (kb_in.p_tail == kb_in.buf + KB_IN_BYTES)
    {
        kb_in.p_tail = kb_in.buf;
    }
    kb_in.count--;
    enable_int();       // 这种成对的就先写好，容易忘
    return scan_code;
}