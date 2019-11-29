/*****************************************************
 * 程序功能：一些宏定义
 * 修改日期：2019.11.28
 */

#ifndef  _YE_CONST_H_
#define	_YE_CONST_H_

//仅仅用来区分局部和全局符号，增加代码可读性，无实际意义
#define PUBLIC
#define PRIVATE static

//GDT和IDT中描述符的个数
#define GDT_SIZE    128

/* 8259A interrupt controller ports. */
#define INT_M_CTL     0x20 /* I/O port for interrupt controller       <Master> */
#define INT_M_CTLMASK 0x21 /* setting bits in this port disables ints <Master> */
#define INT_S_CTL     0xA0 /* I/O port for second interrupt controller<Slave>  */
#define INT_S_CTLMASK 0xA1 /* setting bits in this port disables ints <Slave>  */



#endif
