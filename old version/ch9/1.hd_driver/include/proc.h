/**************************************************************
 * proc.h
 * 程序功能：存放进程相关结构体，比如PCB等以及宏定义
 * 修改日期：2019.12.2
 */
#ifndef _YE_PROC_H_
#define _YE_PROC_H_

#include "protect.h"
//#include "global.h" // 严谨来讲，这里用到了global.h里的宏定义LDT_SIZE就应该包含其头文件
                                        // 但是因为实际上.h不是编译单元，只是在预编译的时候插入源文件开头的
                                        // 所以只要包含这个头文件的源文件里在这个头文件之前包含了global.h头文件即可


/**
 * MESSAGE mechanism is borrowed from MINIX
 * 作者放在了type.h里，但是我觉得进程间通信的消息就
 * 放在proc.h里方便一些
 */
struct mess1 {
	int m1i1;
	int m1i2;
	int m1i3;
	int m1i4;
};
struct mess2 {
	void* m2p1;
	void* m2p2;
	void* m2p3;
	void* m2p4;
};
struct mess3 {
	int	m3i1;
	int	m3i2;
	int	m3i3;
	int	m3i4;
	u64	m3l1;
	u64	m3l2;
	void*	m3p1;
	void*	m3p2;
};
typedef struct {
	int source;
	int type;
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;
} MESSAGE;



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

// 现在为了实现进程调度优先级，添加内容
typedef struct s_proc {
	STACK_FRAME regs;          /* process registers saved in stack frame */

	u16 ldt_sel;               /* gdt selector giving ldt base and limit， 进程LDT的选择子 */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data， LDT */
	
	//	新加的内容，可见进程表的内容是可以自己设计的，TSS则是规定的
	int ticks;				/* remained	ticks , 当前进程剩余可执行的时间片*/
	int priority;		// 等于ticks的初值，始终不变，当所有进程的ticks都变0后又重置为各自的priority继续执行

	u32 pid;                   /* process id passed in from MM ，进程号*/
	char p_name[16];           /* name of the process， 进程名称 */

	// 用于支持IPC新增的几个
	int  p_flags;              /**非零，进程阻塞，=0进程运行
				    * process flags.
				    * A proc is runnable if p_flags==0
					* 非零的值代表其是因RECEIVING还是SENDING被阻塞
					* 因为IPC是同步的所以收和发都要等到完成通信才能继续执行
				    */

	MESSAGE * p_msg;
	int p_recvfrom;
	int p_sendto;

	int has_int_msg;           /**
				    * nonzero if an INTERRUPT occurred when
				    * the task is not ready to deal with it.
				    */

	struct s_proc * q_sending;   /**
				    * queue of procs sending messages to
				    * this proc
				    */
	struct s_proc * next_sending;/**
				    * next proc in the sending
				    * queue (q_sending)
				    */

	int nr_tty;					// 为了将进程与TTY对应添加的。
} PROCESS;

// 用于在进程初始化的时候提供起始地址堆栈等信息
typedef struct s_task {
	task_f	initial_eip;
	int	stacksize;
	char	name[32];
}TASK;

#define proc2pid(x) (x - proc_table)
/* Number of tasks & procs , 分成Tasks和Processes*/
#define NR_TASKS    4 // 进程数
#define NR_PROCS	3

#define STACK_SIZE_TESTA    0x8000
#define STACK_SIZE_TESTB	0x8000
#define STACK_SIZE_TESTC	0x8000
#define	STACK_SIZE_TTY			0x8000
#define STACK_SIZE_SYS		0x8000
#define STACK_SIZE_HD		0x8000	// 新增了两任务
#define STACK_SIZE_FS		0x8000
// 多个进程的栈的总大小
#define STACK_SIZE_TOTAL	(STACK_SIZE_TESTA + \
				STACK_SIZE_TESTB + \
				STACK_SIZE_TESTC + \
				STACK_SIZE_TTY + \
				STACK_SIZE_SYS + \
				STACK_SIZE_HD + \
				STACK_SIZE_FS  	)




#endif