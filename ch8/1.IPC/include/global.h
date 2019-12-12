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

// 定义global.c里, 这里声明过后才能在别的地方引用
extern  PROCESS proc_table[];
extern  char    task_stack[];

extern  TASK    task_table[];
extern  TASK    user_proc_table[];

extern  irq_handler irq_table[];

extern  system_call sys_call_table[NR_SYS_CALL];

extern	TTY	tty_table[NR_CONSOLES]; 
extern	CONSOLE	console_table[NR_CONSOLES];	

#endif