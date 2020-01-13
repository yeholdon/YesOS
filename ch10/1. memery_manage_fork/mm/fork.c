/**************************************************************
 * mm/fork.c            mm = memery manager
 * 程序功能：进程复制（创建）相关
 * 修改日期：2020.1.13
 */

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"


PRIVATE void cleanup(struct s_proc * proc);

/*****************************************************************************
 *                                do_fork
 *****************************************************************************/
/**
 * Perform the fork() syscall.
 * 
 * @return  Zero if success, otherwise -1.
 *****************************************************************************/
PUBLIC int do_fork()
{
	/* find a free slot in proc_table
         1.在进程表数组里找一个空项来存子进程的进程表项 */
	struct s_proc* p = proc_table;
	int i;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++)
		if (p->p_flags == FREE_SLOT)
			break;

	int child_pid = i;
	assert(p == &proc_table[child_pid]);
	assert(child_pid >= NR_TASKS + NR_NATIVE_PROCS);
	if (i == NR_TASKS + NR_PROCS) /* no free slot */
		return -1;
	assert(i < NR_TASKS + NR_PROCS);

	/* duplicate the process table 
    具体步骤：
    先复制父进程的进程表项*/
	int pid = mm_msg.source;
	u16 child_ldt_sel = p->ldt_sel; // 通过
	*p = proc_table[pid];
	p->ldt_sel = child_ldt_sel;
	p->pid_parent = pid;
	sprintf(p->p_name, "%s_%d", proc_table[pid].p_name, child_pid);

	/* duplicate the process: T, D & S 
    再复制父进程的进程映像，通过父进程的ldt获取地址 */
	struct s_descriptor * ppd;

	/* Text segment */
	ppd = &proc_table[pid].ldts[INDEX_LDT_C];
	/* base of T-seg, in bytes */
	int caller_T_base  = reassembly(ppd->base_high, 24,
					ppd->base_mid,  16,
					ppd->base_low);
	/* limit of T-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	int caller_T_limit = reassembly(0, 0,
					(ppd->limit_high_attr2 & 0xF), 16,
					ppd->limit_low);
	/* size of T-seg, in bytes */
	int caller_T_size  = ((caller_T_limit + 1) *
			      ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
			       4096 : 1));

	/* Data & Stack segments */
	ppd = &proc_table[pid].ldts[INDEX_LDT_RW];
	/* base of D&S-seg, in bytes */
	int caller_D_S_base  = reassembly(ppd->base_high, 24,
					  ppd->base_mid,  16,
					  ppd->base_low);
	/* limit of D&S-seg, in 1 or 4096 bytes,
	   depending on the G bit of descriptor */
	int caller_D_S_limit = reassembly((ppd->limit_high_attr2 & 0xF), 16,
					  0, 0,
					  ppd->limit_low);
	/* size of D&S-seg, in bytes */
	int caller_D_S_size  = ((caller_T_limit + 1) *
				((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
				 4096 : 1));

	/* we don't separate T, D & S segments, so we have: */
	assert((caller_T_base  == caller_D_S_base ) &&
	       (caller_T_limit == caller_D_S_limit) &&
	       (caller_T_size  == caller_D_S_size ));

	/* base of child proc, T, D & S segments share the same space,
	   so we allocate memory just once 
       2. 分配内存*/
	int child_base = alloc_mem(child_pid, caller_T_size);
	/* int child_limit = caller_T_limit; */
	printl("{MM} 0x%x <- 0x%x (0x%x bytes)\n",
	       child_base, caller_T_base, caller_T_size);
	/* child is a copy of the parent */
	phys_copy((void*)child_base, (void*)caller_T_base, caller_T_size);

	/* child's LDT 因为父子进程的内存位置不同所以要更新下LDT*/
	init_descriptor(&p->ldts[INDEX_LDT_C],
		  child_base,
		  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
	init_descriptor(&p->ldts[INDEX_LDT_RW],
		  child_base,
		  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

	/* tell FS, see fs_fork() 
    3. 通知FS，处理父子进程共享已打开文件的问题*/
	MESSAGE msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = child_pid;
	send_recv(BOTH, TASK_FS, &msg2fs);

	/* child PID will be returned to the parent proc
    4. fork函数返回，由于父进程此时由于等待fork的返回被挂起
    而子进程由于复制了进程表也是挂起状态，所以返回前也要发信号
    让子进程解除阻塞，同时也把子进程fork的返回值（0）发回去 */
	mm_msg.PID = child_pid;     // 这个会由mm_task发回

	/* birth of the child */
	MESSAGE m;
	m.type = SYSCALL_RET;
	m.RETVAL = 0;
	m.PID = 0;
	send_recv(SEND, child_pid, &m);

	return 0;
}