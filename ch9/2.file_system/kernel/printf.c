/**************************************************************
 * printf.c
 * 程序功能：C库函数printf
 * 修改日期：2019.12.8
 */

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

/*======================================================================*
                                 printf:格式化显示可变数量变量的C库函数
 *======================================================================*/
int printf(const char *fmt, ...)        // 这个其实不算操作系统的内容，就按普通函数，不加PUBLIC了
{
    int i; //表示可变参数的数量
    char buf[STR_DEFAULT_LEN];      // 存格式化好的要输出的内容

    va_list arg = (va_list)((char *)(&fmt) + 4);        // va_list=char *。获取后面可变参数列表的首地址
    i = vsprintf(buf, fmt, arg);
    // 要输出的内容有了， 长度也有了，调用系统调用来借助系统输出
    // write(buf, i);
    // 这里的printf调用的系统调用不再是write而是另外一个特殊的printx
    // 可以处理assert和panic错误
    buf[i] = 0;
    printx(buf);
    // assert(0);
    // int c = write(1, buf, i);
    // assert(c == i);

    return i;
}


/*****************************************************************************
 *                                printl
 *****************************************************************************/
/**
 * low level print
 * 
 * @param fmt  The format string
 * 
 * @return  The number of chars printed.
 *****************************************************************************/
PUBLIC int printl(const char *fmt, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /**
						     * 4: size of `fmt' in
						     *    the stack
						     */
	i = vsprintf(buf, fmt, arg);
	printx(buf);

	return i;
}
