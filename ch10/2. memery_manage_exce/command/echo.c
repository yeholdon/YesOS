/**************************************************************
 * echo.c
 * 程序功能：自己实现的应用程序，需要调用CRT
 * 修改日期：2020.1.15
 */

#include "stdio.h"

int main(int argc, char *argv[]) 
{
    for (int i = 0; i < argc; i++)
    {
        // printf("%s%s", i == 1 ? "" : " ", argv[i]);
    }
    // printf("\n");

    return 0;
}