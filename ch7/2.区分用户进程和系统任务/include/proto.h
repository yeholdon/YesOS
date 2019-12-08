/**************************************************************
 * proto.h
 * 程序功能：存放各个主要函数的声明
 * 修改日期：2019.11.28
 */
#ifndef _YES_PROTO_H_
#define _YES_PROTO_H_

#include "tty.h"
#include "console.h"
#include "proc.h"
#include "protect.h"

// kliba.asm
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void disp_int(int input);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);
PUBLIC  void disable_irq(int irq);  // 按照常理，这俩应该在这里声明了才能用，但是貌似不声明也可，等会测试一下
PUBLIC  void enable_irq(int irq);
PUBLIC  void enable_int();
PUBLIC  void disable_int();

// protect.c
PUBLIC void	init_prot();
PUBLIC void	init_8259A();
PUBLIC u32	seg2phys(u16 seg);

// klib.c 
PUBLIC void	delay(int time);
PUBLIC char * itoa(char *str, int num);

// kernel.asm 
void restart();

// main.c 
void TestA();
void TestB();
void TestC();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);
PUBLIC  void    init_clock();

/* keyboard.c */
PUBLIC void keyboard_handler(int irq);
PUBLIC void init_keyboard();
PUBLIC  void keyboard_read(TTY *p_tty);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC  void    milli_delay(int milli_delay);
PUBLIC void schedule();

/* 以下是系统调用相关 */
// 系统调用：系统级
/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC  int     sys_write(char* buf, int len, PROCESS* p_proc);
/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */

// 系统调用：用户级
PUBLIC  int     get_ticks();
PUBLIC  void    write(char* buf, int len);

/* tty */
PUBLIC  void task_tty();
PUBLIC  void in_process(TTY *p_tty, u32 key);
/* console */
PUBLIC int is_curent_console(CONSOLE *);
PUBLIC void out_char(CONSOLE *p_con, char ch);
PUBLIC void init_screen(TTY *p_tty);
PUBLIC  void select_console(int nr_console);
PUBLIC  void scroll_screen(CONSOLE *p_con, int direction);

// C库函数
/* printf.c */
PUBLIC  int     printf(const char *fmt, ...);

/* vsprintf.c */
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);

#endif