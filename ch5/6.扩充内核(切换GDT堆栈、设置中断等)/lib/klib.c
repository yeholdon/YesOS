/*****************************************************
 * klib.c
 * 程序功能：进制转换和显示等额外功能函数
 * 修改日期：2019.11.30
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "global.h"


/*======================================================================*
                               itoa:把int转成一位位的16进制字符显示出来，相比C标准库里的简单许多
 *======================================================================*/
/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */

PUBLIC char *itoa(char *str, int num) 
{
    char *p = str;
    char ch;
    int i;
    int flag = 0;

    *p++ = '0';
    *p++ = 'x';

    if(num == 0) {
        *p++ = '0';
    }
    else {
        for(i = 28; i >= 0; i -= 4) {
            ch = (num >> i) & 0xF;
            if(flag  || ch > 0) {       // 前面的0不显示
                flag = 1;
                ch += '0';
                if(ch > '9') {
                    ch += 7;
                }
                *p++ = ch;
            }
        }
    }
    *p = 0;     //最后补的这个0好像没啥意义，就是一个结束符
    return str;
}

/*======================================================================*
                               disp_int
 *======================================================================*/
PUBLIC void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}
