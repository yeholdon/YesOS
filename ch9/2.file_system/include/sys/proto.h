/**************************************************************
 * proto.h
 * 程序功能：存放各个主要函数的声明
 * 修改日期：2019.11.28
 */
#ifndef _YE_PROTO_H_
#define _YE_PROTO_H_

#include "tty.h"
#include "console.h"
#include "proc.h"
#include "protect.h"
#include "const.h"

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
PUBLIC void	port_read(u16 port, void* buf, int n);
PUBLIC void port_write(u16 port, void* buf, int n);
// protect.c
PUBLIC void	init_prot();
PUBLIC void	init_8259A();
PUBLIC u32	seg2phys(u16 seg);

// klib.c 
PUBLIC void	delay(int time);
PUBLIC char * itoa(char *str, int num);
PUBLIC int max(int a, int b);
PUBLIC int min(int a, int b);

// kernel.asm 
void restart();

// main.c 
void TestA();
void TestB();
void TestC();
PUBLIC void panic(const char *fmt, ...);

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
PUBLIC  int     sys_write(int unused1, char* buf, int len, PROCESS* p_proc);
PUBLIC  int     sys_sendrec(int function, int src_dest, MESSAGE *m, struct s_proc *p);
PUBLIC	int	    sys_printx(int _unused1, int _unused2, char* s, struct s_proc * p_proc);

PUBLIC	void*	va2la(int pid, void* va);   
PUBLIC	int	ldt_seg_linear(struct s_proc* p, int idx);
PUBLIC	void	reset_msg(MESSAGE* p);
PUBLIC	void	dump_msg(const char * title, MESSAGE* m);
PUBLIC	void	dump_proc(struct s_proc * p);
PUBLIC	int	send_recv(int function, int src_dest, MESSAGE* msg);
PUBLIC void	inform_int(int task_nr);

/* lib/misc.c */
PUBLIC int memcmp(const void * s1, const void *s2, int n);
PUBLIC void spin(char * func_name);


/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */

// 系统调用：用户级
PUBLIC  int     get_ticks();
PUBLIC  void    write_syscall_version(char* buf, int len);
PUBLIC	int	printx(char* str);
PUBLIC  int sendrec(int function, int src_dest, MESSAGE *p_msg);

/* system task */
PUBLIC void task_sys();

/* kernel/hd.c */
PUBLIC void	task_hd();
PUBLIC void	hd_handler(int irq);

/* fs/main.c */
PUBLIC void			task_fs();
PUBLIC int			rw_sector(int io_type, int dev, u64 pos,
					  int bytes, int proc_nr, void * buf);
PUBLIC struct inode *		get_inode(int dev, int num);
PUBLIC void			put_inode(struct inode * pinode);
PUBLIC void			sync_inode(struct inode * p);
PUBLIC struct super_block *	get_super_block(int dev);

/* fs/open.c */
PUBLIC int		do_open();
PUBLIC int		do_close();

/* fs/read_write.c */
PUBLIC int		do_rdwt();

/* fs/link.c */
PUBLIC int		do_unlink();

/* fs/misc.c */
PUBLIC int		do_stat();
PUBLIC int		strip_path(char * filename, const char * pathname,
				   struct inode** ppinode);
PUBLIC int		search_file(char * path);

/* fs/disklog.c */
PUBLIC int		do_disklog();
PUBLIC int		disklog(char * logstr); /* for debug */
PUBLIC void		dump_fd_graph(const char * fmt, ...);

/* tty */
PUBLIC  void task_tty();
PUBLIC  void in_process(TTY *p_tty, u32 key);
/* console */
PUBLIC int is_current_console(CONSOLE *);
PUBLIC void out_char(CONSOLE *p_con, char ch);
PUBLIC void init_screen(TTY *p_tty);
PUBLIC  void select_console(int nr_console);
PUBLIC  void scroll_screen(CONSOLE *p_con, int direction);



// C库函数
/* printf.c */
PUBLIC  int     printf(const char *fmt, ...);
#define	printl	printf  // printl宏定义为printf

/* vsprintf.c */
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif