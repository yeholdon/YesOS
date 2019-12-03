/**************************************************************
 * proc.h
 * 程序功能：存放进程相关结构体，比如PCB等以及宏定义
 * 修改日期：2019.12.2
 */
#ifndef _YE_PROC_H_
#define _YE_PROC_H_

//#include "global.h" // 严谨来讲，这里用到了global.h里的宏定义LDT_SIZE就应该包含其头文件
                                        // 但是因为实际上.h不是编译单元，只是在预编译的时候插入源文件开头的
                                        // 所以只要包含这个头文件的源文件里在这个头文件之前包含了global.h头文件即可

// 定义PCB中栈内容也就是各个寄存器的值
typedef struct s_stackframe {
	u32	gs;		/* \                                    */
	u32	fs;		/* |                                    */
	u32	es;		/* |                                    */
	u32	ds;		/* |                                    */
	u32	edi;		/* |                                    */
	u32	esi;		/* | pushed by save()                   */
	u32	ebp;		/* |                                    */
	u32	kernel_esp;	/* <- 'popad' will ignore it            */
	u32	ebx;		/* |                                    */
	u32	edx;		/* |                                    */
	u32	ecx;		/* |                                    */
	u32	eax;		/* /                                    */
	u32	retaddr;	/* return addr for kernel.asm::save()   */
    // 后面几个是由CPU push的，前面是是自己手动push的
	u32	eip;		/* \                                    */
	u32	cs;		/* |                                    */
	u32	eflags;		/* | pushed by CPU during interrupt     */
	u32	esp;		/* |                                    */
	u32	ss;		/* /                                    */
} STACK_FRAME;


typedef struct s_proc {
	STACK_FRAME regs;          /* process registers saved in stack frame */

	u16 ldt_sel;               /* gdt selector giving ldt base and limit， 进程LDT的选择子 */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data， LDT */
	u32 pid;                   /* process id passed in from MM ，进程号*/
	char p_name[16];           /* name of the process， 进程名称 */
} PROCESS;

// 用于在进程初始化的时候提供起始地址堆栈等信息
typedef struct s_task {
	task_f	initial_eip;
	int	stacksize;
	char	name[32];
}TASK;



#define NR_TASKS    2   // 进程数，先改成2个
#define STACK_SIZE_TESTA    0x8000
#define STACK_SIZE_TESTB	0x8000
// 多个进程的栈的总大小
#define STACK_SIZE_TOTAL	(STACK_SIZE_TESTA + \
				STACK_SIZE_TESTB)

#endif