/*****************************************************
 * 程序功能：一些宏定义
 * 修改日期：2019.11.28
 */

#ifndef  _YE_CONST_H_
#define	_YE_CONST_H_

/* EXTERN is defined as extern except in global.c , 保证全局变量只有一份定义，其余都是声明*/
#define EXTERN extern

//仅仅用来区分局部和全局符号，增加代码可读性，无实际意义
#define PUBLIC
#define PRIVATE static

//GDT和IDT中描述符的个数
#define GDT_SIZE    128
#define IDT_SIZE    256

/* 权限, 暂时用在初始化描述符上 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/* 8259A interrupt controller ports. */
#define INT_M_CTL     0x20 /* I/O port for interrupt controller       <Master> */
#define INT_M_CTLMASK 0x21 /* setting bits in this port disables ints <Master> */
#define INT_S_CTL     0xA0 /* I/O port for second interrupt controller<Slave>  */
#define INT_S_CTLMASK 0xA1 /* setting bits in this port disables ints <Slave>  */



#endif
