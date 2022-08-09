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
                                i2a
 *======================================================================*/
PRIVATE char* i2a(int val, int base, char ** ps)
{
	int m = val % base;
	int q = val / base;
	if (q) {
		i2a(q, base, ps);
	}
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps;
}


/*======================================================================*
                                vsprintf:printf参数列表的处理函数
 *======================================================================*/
// int vsprintf(char *buf, const char *fmt, va_list args)
// {
//     char *p;
//     char tmp[256];
//     va_list p_next_arg = args;         // 为了能够更好地说明这个遍变量的用途，才用了va_list

//     for (p = buf; *fmt; fmt++) {
//         // p是分解出来后保存的目标缓冲区的指针，fmt是被分解的源字符串的指针
//         if(*fmt != '%') {       // 这种处理确实没有printf那么全面完美，但是目前足够了
//             *p++ = *fmt;
//             continue;
//         }
//         fmt++;

//         switch (*fmt)
//         {
//         case 'x':          // %x:16进制整数
//             itoa(tmp, *((int *) p_next_arg));   // 将整型转成16进制字符串形式存在tmp里
//             strcpy(p, tmp);                                     // 转换好后copy完到目标buf里
//             p_next_arg += 4;                                //  从%x知道类型是整型的，所以4个字节
//             p += strlen(tmp);
//             break;
//         case 's':                   // 这个先不实现
//             break;
//         default:
//             break;
//         }
//     }

//     return (p - buf);   // 返回的是可变参数的数量
// }

PUBLIC int vsprintf(char *buf, const char *fmt, va_list args)
{
	char*	p;

	va_list	p_next_arg = args;
	int	m;

	char	inner_buf[STR_DEFAULT_LEN];
	char	cs;
	int	align_nr;

	for (p=buf;*fmt;fmt++) {
		if (*fmt != '%') {
			*p++ = *fmt;
			continue;
		}
		else {		/* a format string begins */
			align_nr = 0;
		}

		fmt++;

		if (*fmt == '%') {
			*p++ = *fmt;
			continue;
		}
		else if (*fmt == '0') {
			cs = '0';
			fmt++;
		}
		else {
			cs = ' ';
		}
		while (((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9')) {
			align_nr *= 10;
			align_nr += *fmt - '0';
			fmt++;
		}

		char * q = inner_buf;
		memset(q, 0, sizeof(inner_buf));

		switch (*fmt) {
		case 'c':
			*q++ = *((char*)p_next_arg);
			p_next_arg += 4;
			break;
		case 'x':
			m = *((int*)p_next_arg);
			i2a(m, 16, &q);
			p_next_arg += 4;
			break;
		case 'd':
			m = *((int*)p_next_arg);
			if (m < 0) {
				m = m * (-1);
				*q++ = '-';
			}
			i2a(m, 10, &q);
			p_next_arg += 4;
			break;
		case 's':
			strcpy(q, (*((char**)p_next_arg)));
			q += strlen(*((char**)p_next_arg));
			p_next_arg += 4;
			break;
		default:
			break;
		}

		int k;
		for (k = 0; k < ((align_nr > strlen(inner_buf)) ? (align_nr - strlen(inner_buf)) : 0); k++) {
			*p++ = cs;
		}
		q = inner_buf;
		while (*q) {
			*p++ = *q++;
		}
	}

	*p = 0;

	return (p - buf);
}


/*======================================================================*
                                 sprintf
 *======================================================================*/
// int sprintf(char *buf, const char *fmt, ...)
// {
// 	va_list arg = (va_list)((char*)(&fmt) + 4);        /* 4 是参数 fmt 所占堆栈中的大小 */
// 	return vsprintf(buf, fmt, arg);
// }
