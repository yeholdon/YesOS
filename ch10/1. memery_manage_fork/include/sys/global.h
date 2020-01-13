/**************************************************************
 * global.h
 * 程序功能：存放全局变量的声明
 * 修改日期：2019.11.29
 */
#ifndef _YE_GLOBAL_H_
#define _YE_GLOBAL_H_

#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "stdio.h"  // 这个头文件要放在前面才行，否则会出现EXTERN重定义
/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif




EXTERN  int ticks;

EXTERN	int		disp_pos;
EXTERN  u8  gdt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN DESCRIPTOR  gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	GATE		idt[IDT_SIZE];

EXTERN  u32 k_reenter;

EXTERN  PROCESS *p_proc_ready;
EXTERN  TSS tss;

EXTERN	int		nr_current_console;

EXTERN	int	key_pressed; /**
			      * used for clock_handler
			      * to wake up TASK_TTY when
			      * a key is pressed
			      */

// 定义global.c里, 这里声明过后才能在别的地方引用
extern  PROCESS proc_table[];
extern  char    task_stack[];

extern  TASK    task_table[];
extern  TASK    user_proc_table[];

extern  irq_handler irq_table[];

extern  system_call sys_call_table[NR_SYS_CALL];

extern	TTY	tty_table[NR_CONSOLES]; 
extern	CONSOLE	console_table[NR_CONSOLES];	

/* MM */
EXTERN	MESSAGE			mm_msg;
extern	u8 *			mmbuf;
extern	const int		MMBUF_SIZE;
EXTERN	int			memory_size;

// fs相关
extern  struct dev_drv_map dev_to_dri_map[];
extern	u8 *			fsbuf;
extern	const int		FSBUF_SIZE;
EXTERN	struct file_desc	f_desc_table[NR_FILE_DESC];
EXTERN	struct inode		inode_table[NR_INODE];
EXTERN	struct super_block	super_block[NR_SUPER_BLOCK];

EXTERN	MESSAGE			fs_msg;
EXTERN	struct s_proc *		pcaller;
EXTERN	struct inode *		root_inode;

#endif