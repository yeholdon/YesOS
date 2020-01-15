/**************************************************************
 * stdio.h
 * 程序功能：主要是文件IO相关的宏定义和库函数声明
 * 修改日期：2020.1.9
 */
#ifndef _YE_STDIO_H_
#define _YE_STDIO_H_
#include "type.h"
/* the assert macro */
#define ASSERT			//要assert起作用的时候加上
#ifdef	ASSERT
void	assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)  if (exp) ; \
        else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)// #加在宏定义的参数前：给参数添加""
// _FILE_等为编译器内置的宏
#else
#define	assert(exp)
#endif

/* EXTERN is defined as extern except in global.c , 保证全局变量只有一份定义，其余都是声明*/
#define EXTERN extern



#define	STR_DEFAULT_LEN	1024

// 应该是创建文件还是读写文件的标志，和读写文件的消息FLAGS对应
#define	O_CREAT		1
#define	O_RDWR		2

#define SEEK_SET	1
#define SEEK_CUR	2
#define SEEK_END	3

#define	MAX_PATH	128     // 文件路径的最大字符长度

/* printf.c */
PUBLIC  int     printf(const char *fmt, ...);
PUBLIC  int     printl(const char *fmt, ...);

/* vsprintf.c */
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);
PUBLIC	int	sprintf(char *buf, const char *fmt, ...);

/*--------*/
/* 库函数 */
/*--------*/
#ifdef ENABLE_DISK_LOG
#define SYSLOG syslog
#endif

/* lib/open.c */
PUBLIC	int	open(const char *pathname, int flags);

/* lib/close.c */
PUBLIC	int	close(int fd);

/* lib/read.c */
PUBLIC int	read		(int fd, void *buf, int count);

/* lib/write.c */
PUBLIC int	write		(int fd, const void *buf, int count);

/* lib/getpid.c */
PUBLIC int	getpid		();

/* lib/unlink.c */
PUBLIC	int	unlink		(const char *pathname);

/* lib/fork.c */
PUBLIC int	fork		();

/* lib/exit.c */
PUBLIC void	exit		(int status);

/* lib/wait.c */
PUBLIC int	wait		(int * status);

/* lib/syslog.c */
PUBLIC	int	syslog		(const char *fmt, ...);


#endif