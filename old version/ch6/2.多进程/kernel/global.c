/**************************************************************
 * global.c
 * 程序功能：存放全局变量的定义，保证只有包含在这里的一份是定义
 * 修改日期：2019.11.29
 */

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include  "proc.h"
#include "global.h"

PUBLIC  PROCESS proc_table[NR_TASKS];

PUBLIC  char    task_stack[STACK_SIZE_TOTAL];

// 这个是对应s_task的数组，用来批量初始化proc_table用的
PUBLIC	TASK	task_table[NR_TASKS] = {{TestA, STACK_SIZE_TESTA, "TestA"},
					{TestB, STACK_SIZE_TESTB, "TestB"}};
