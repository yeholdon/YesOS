/**************************************************************
 * console.h
 * 程序功能：控制台结构定义
 * 修改日期：2019.12.7
 */

#ifndef _YES_CONSOLE_H_
#define _YES_CONSOLE_H_

typedef struct s_console
{
	unsigned int	current_start_addr; 	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		           /* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		         /* 当前控制台占的显存大小 */
	unsigned int	cursor;			                       /* 当前光标位置 */  
} CONSOLE;


#define SCR_UP	1	/* scroll forward */
#define SCR_DOWN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

#define DEFAULT_CHAR_COLOR	0x07	/* 默认属性:0000 0111 黑底白字 */
#endif