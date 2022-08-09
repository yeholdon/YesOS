/**************************************************************
 * string.h
 * 程序功能：模仿C语言库的string.h放字符串操作相关的函数
 * 修改日期：2019.11.28
 */
#ifndef _YES_STRING_H_
#define _YES_STRING_H_

#include "const.h"

PUBLIC	void*	memcpy(void* p_dst, void* p_src, int size);
PUBLIC	void	memset(void* p_dst, char ch, int size);
PUBLIC  char* strcpy(char* p_dst, char* p_src);
PUBLIC  int strlen(char* p_str);


#endif