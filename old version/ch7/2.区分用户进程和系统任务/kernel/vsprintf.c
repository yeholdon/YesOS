/**************************************************************
 * vsprintf.c
 * 程序功能：C库函数vsprintf
 * 修改日期：2019.12.8
 */
#include "type.h"
#include "const.h"
#include "string.h"
#include "proto.h"

/*======================================================================*
                                vsprintf:printf参数列表的处理函数
 *======================================================================*/
int vsprintf(char *buf, const char *fmt, va_list args)
{
    char *p;
    char tmp[256];
    va_list p_next_arg = args;         // 为了能够更好地说明这个遍变量的用途，才用了va_list

    for (p = buf; *fmt; fmt++) {
        // p是分解出来后保存的目标缓冲区的指针，fmt是被分解的源字符串的指针
        if(*fmt != '%') {       // 这种处理确实没有printf那么全面完美，但是目前足够了
            *p++ = *fmt;
            continue;
        }
        fmt++;

        switch (*fmt)
        {
        case 'x':          // %x:16进制整数
            itoa(tmp, *((int *) p_next_arg));   // 将整型转成16进制字符串形式存在tmp里
            strcpy(p, tmp);                                     // 转换好后copy完到目标buf里
            p_next_arg += 4;                                //  从%x知道类型是整型的，所以4个字节
            p += strlen(tmp);
            break;
        case 's':                   // 这个先不实现
            break;
        default:
            break;
        }
    }

    return (p - buf);   // 返回的是可变参数的数量
}