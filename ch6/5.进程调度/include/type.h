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

typedef void *system_call;      // 为什么这里不是函数指针的定义法？因为系统调用有不同是函数类型，各个系统调用函数的参数列表都不同
                                                            // 所以不能直接定义成带参数列表的函数指针，而是不能带参数列表的void型指针，这样可以适应任何类型的函数

#endif /* _YE_TYPE_H_ */