/**************************************************************
 * proto.h
 * 程序功能：存放各个主要函数的声明
 * 修改日期：2019.11.28
 */

// kliba.asm
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void disp_int(int input);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);
PUBLIC  void disable_irq(int irq);  // 按照常理，这俩应该在这里声明了才能用，但是貌似不声明也可，等会测试一下
PUBLIC  void enable_irq(int irq);

// protect.c
PUBLIC void	init_prot();
PUBLIC void	init_8259A();
PUBLIC u32	seg2phys(u16 seg);

// klib.c 
PUBLIC void	delay(int time);

// kernel.asm 
void restart();

// main.c 
void TestA();
void TestB();
void TestC();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);


/* 以下是系统调用相关 */
/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */

/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();

