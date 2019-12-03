/**************************************************************
 * global.h
 * 程序功能：存放全局变量的声明
 * 修改日期：2019.11.29
 */
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "proc.h"
/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		disp_pos;
EXTERN  u8  gdt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN DESCRIPTOR  gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	GATE		idt[IDT_SIZE];

EXTERN  u32 k_reenter;

EXTERN  PROCESS *p_proc_ready;
EXTERN  TSS tss;
// 定义global.c里, 这里声明过后才能在别的地方引用
extern  PROCESS proc_table[];
extern  char    task_stack[];

extern  TASK    task_table[];
#endif