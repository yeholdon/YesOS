/*****************************************************
 * 程序功能：类型的typedef
 * 修改日期：2019.11.28
 */

#ifndef	_YE_TYPE_H_
#define	_YE_TYPE_H_

typedef	unsigned int		u32;
typedef	unsigned short		u16;
typedef	unsigned char		u8;

typedef void (*int_handler) ();     // 这个是为了初始化idt中描述符时获取各个中断（异常）处理函数的基地址用的函数指针
typedef void (*task_f) ();
typedef void (*irq_handler) (int irq); // 和clock_handler一致的，中断处理程序
#endif /* _YE_TYPE_H_ */