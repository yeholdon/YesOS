/**************************************************************
 * mm/wait.c            mm = memery manager
 * 程序功能：进程退出相关
 * 修改日期：2020.1.15
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
#include "stdio.h"

PUBLIC void cleanup(struct s_proc * proc);


/*****************************************************************************
 *                                do_wait
 *****************************************************************************/
/**
 * Perform the wait() syscall.
 *
 * If proc P calls wait(), then MM will do the following in this routine:
 *     <1> iterate proc_table[],
 *         if proc A is found as P's child and it is HANGING
 *           - reply to P (cleanup() will send P a messageto unblock it)
 *             {P's wait() call is done}
 *           - release A's proc_table[] entry
 *             {A's exit() call is done}
 *           - return (MM will go on with the next message loop)
 *     <2> if no child of P is HANGING
 *           - set P's WAITING bit
 *             {things will be done at do_exit()::comment::<5>::(1)}
 *     <3> if P has no child at all
 *           - reply to P with error
 *             {P's wait() call is done}
 *     <4> return (MM will go on with the next message loop)
 *
 *****************************************************************************/
PUBLIC void do_wait()
{
	printl("{MM} ((--do_wait()--))");
	/* dump_fd_graph("((--do_wait()--))"); */
	int pid = mm_msg.source;

	int i;
	int children = 0;
	struct s_proc* p_proc = proc_table;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p_proc++) {
		if (p_proc->pid_parent == pid) {
			children++;
			if (p_proc->p_flags & HANGING) {
				// printl("{MM} ((--do_wait():: %s (%d) is HANGING, "
				//        "so let's clean it up.--))",
				    //    p_proc->p_name, i);
				/* dump_fd_graph("((--do_wait():: %s (%d) is HANGING, " */
				/*        "so let's clean it up.--))", */
				/*        p_proc->name, i); */
				cleanup(p_proc);
				return;
			}
		}
	}

	if (children) {
		/* has children, but no child is HANGING */
		proc_table[pid].p_flags |= WAITING;
		// printl("{MM} ((--do_wait():: %s (%d) is WAITING for child "
		//        "to exit().--))\n", proc_table[pid].p_name, pid);
		/* dump_fd_graph("((--do_wait():: %s (%d) is WAITING for child " */
		/*        "to exit().--))", proc_table[pid].name, pid); */
	}
	else {
		/* no child at all */
		printl("{MM} ((--do_wait():: %s (%d) has no child at all.--))\n",
		       proc_table[pid].p_name, pid);
		/* dump_fd_graph("((--do_wait():: %s (%d) is has no child at all.--))", */
		/*        proc_table[pid].name, pid); */
		MESSAGE msg;
		msg.type = SYSCALL_RET;
		msg.PID = NO_TASK;
		send_recv(SEND, pid, &msg);
	}
}
