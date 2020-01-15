/**************************************************************
 * main.c
 * 程序功能：一个简单进程体，即一个函循环打印字符的函数
 * 修改日期：2019.12.2
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "stdio.h"

/*======================================================================*
                            kernel_main:内核主函数
 *======================================================================*/
PUBLIC  int kernel_main() 
{
   disp_str("-----\"kernel_main\" begins-----\n");

    // 下面初始化设置进程表
    TASK *p_task = task_table;
    PROCESS *p_proc = proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;  // 初始位置是栈底
	u16		selector_ldt	= SELECTOR_LDT_FIRST;


	// 要区分用户进程和系统任务的特权级等，所以特权级相关的几项属性要单独设置，而不是统一一样了
	u8	privilege;
	u8	rpl;
	int eflags;
	int   priority;
    for(int i=0;i<NR_TASKS + NR_PROCS;i++, p_proc++, p_task++){		// 这里要分成用户进程和任务
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p_proc->p_flags = FREE_SLOT;
			continue;
		}
		if (i < NR_TASKS)		//先设置系统任务特权级
		{
			p_task = task_table + i;
			privilege = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202;  // IF = 1, IOPL = 1
			priority = 15;
		}
		else
		{
			p_task = user_proc_table + (i - NR_TASKS);
			privilege = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags = 0x202;		// IF = 1, IOPL = 0，用户进程没有IO权限
			priority = 5;
		}
		
		// p_proc->p_flags = 1;		

		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid_parent = NO_TASK;			// pid直接设为循环变量

		p_proc->ldt_sel = selector_ldt;     // idt选择子先设好了，另外还要生成其GDT里的描述符

		if (strcmp(p_task->name, "INIT") != 0) 
		{
			// IDT的生成
			// memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			// 		sizeof(DESCRIPTOR));
			// p_proc->ldts[0].attr1 = DA_C | privilege<< 5;									// 更新
			// memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			// 		sizeof(DESCRIPTOR));	
			// p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;							// 更新

			p_proc->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p_proc->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p_proc->ldts[INDEX_LDT_C].attr1  = DA_C   | privilege << 5;
			p_proc->ldts[INDEX_LDT_RW].attr1 = DA_DRW | privilege<< 5;
		}
		else
		{
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_descriptor(&p_proc->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | privilege<< 5);

			init_descriptor(&p_proc->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | privilege << 5);
		}
		
		

		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;																				// 更新
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| rpl;

        // TASK里的是几个关键的每个进程不一样的信息,进程入口地址、堆栈栈顶和进程名
		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;  // 更新

		p_proc->nr_tty = 0;				// 默认进程与tty0绑定，后面再修改
		// 这一波IPC相关的进程属性初始化别忘记
		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		p_proc->ticks = p_proc->priority = priority;

		for (int j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_task_stack -= p_task->stacksize;

		selector_ldt += 1 << 3;
	}


	// 初始化后重新赋值进程的tty, 由于后面将tty纳入了文件系统，所以这里的初值决定的就是打开tty文件之前的，打开之后就没用了
	// 所以作者的源码里这里就没有再特意赋值，而是保持默认值0
	proc_table[NR_TASKS + 0].nr_tty = 0;
	proc_table[NR_TASKS + 1].nr_tty = 0;
	proc_table[NR_TASKS + 2].nr_tty = 1;
	proc_table[NR_TASKS + 3].nr_tty = 2;

	k_reenter = 0;			// 为了统一，现在在第一个进程执行前，也让k_reenter自增了，所以这里k_reenter初值要改一下
    ticks = 0;  
	p_proc_ready	= proc_table; //一个指向下一个要启动进程的进程表的指针，在kernel.asm中导入使用
	

	// 初始化键盘
	init_keyboard();

    // 初始化8253 PIT
	init_clock();

	// disp_pos = 0;
	// for (int i = 0; i < 80*25; i++) {
	// 	disp_str(" ");
	// }
	// disp_pos = 0;

	restart();                                          // kernel.asm中的函数
    while(1) 
    {
        //暂时先死循环
    }
}



/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	while (1) {
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]\n");
			return;
		}
		printf("    %s (%d bytes)\n", phdr->name, f_len);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / (u32)SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	close(fd);

	printf(" done]\n");
}


/*****************************************************************************
 *                                Init:所有进程的祖先进程 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");

#if 0
	int pid = fork();
	if (pid != 0) { /* parent process */
		printf("parent is running, child pid:%d\n", pid);
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}
	else {	/* child process */
		printf("child is running, pid:%d\n", getpid());
		exit(123);
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}
#endif
	spin("Init\n");
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{

	// int fd;
	// int i, n;
	// const char filename[MAX_FILENAME_LEN+1] = "blah";
	// const char bufw[] = "abcde";
	// const int rd_bytes = 3;
	// char bufr[rd_bytes];

	// assert(rd_bytes <= strlen(bufw));

	// /* create */
	// fd = open(filename, O_CREAT | O_RDWR);
	// assert(fd != -1);
	// printl("File created: %s (fd %d)\n", filename, fd);

	// /* write */
	// n = write(fd, bufw, strlen(bufw));
	// assert(n == strlen(bufw));

	// /* close */
	// close(fd);

	// /* open */
	// fd = open(filename, O_RDWR);
	// assert(fd != -1);
	// printl("File opened. fd: %d\n", fd);

	// /* read */
	// n = read(fd, bufr, rd_bytes);
	// assert(n == rd_bytes);
	// bufr[n] = 0;
	// printl("%d bytes read: %s\n", n, bufr);

	// /* close */
	// close(fd);

	// char * filenames[] = {"/foo", "/bar", "/baz"};

	// /* create files */
	// for (i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) {
	// 	fd = open(filenames[i], O_CREAT | O_RDWR);
	// 	assert(fd != -1);
	// 	printl("File created: %s (fd %d)\n", filenames[i], fd);
	// 	close(fd);
	// }

	// char * rfilenames[] = {"/bar", "/foo", "/baz", "/dev_tty0"};

	// /* remove files */
	// for (i = 0; i < sizeof(rfilenames) / sizeof(rfilenames[0]); i++) {
	// 	if (unlink(rfilenames[i]) == 0)
	// 		printl("File removed: %s\n", rfilenames[i]);
	// 	else
	// 		printl("Failed to remove file: %s\n", rfilenames[i]);
	// }



	spin("TestA");
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{

	// char tty_name[] = "/dev_tty1";

	// int fd_stdin  = open(tty_name, O_RDWR);
	// assert(fd_stdin  == 0);
	// int fd_stdout = open(tty_name, O_RDWR);
	// assert(fd_stdout == 1);

	// char rdbuf[128];
	// // while(1){
	// // 	printl("B");
	// // 	milli_delay(2000);
	// // }
	// while (1) {  
	// 	printf("[Ye's OS-TTY #1]-$ ");
	// 	int r = read(fd_stdin, rdbuf, 70);
	// 	rdbuf[r] = 0;

	// 	if (strcmp(rdbuf, "hello") == 0)
	// 		printf("hello world!\n");
	// 	else
	// 		if (rdbuf[0])
	// 			printf("{%s}\n", rdbuf);
	// }



	spin("TestB");
	// assert(0); /* never arrive here */
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{


	// char tty_name[] = "/dev_tty2";

	// int fd_stdin  = open(tty_name, O_RDWR);
	// // printl("fd_stdin:%d\n", fd_stdin);
	// assert(fd_stdin  == 0);
	// int fd_stdout = open(tty_name, O_RDWR);
	// // printl("fd_stdout:%d\n", fd_stdout);
	// assert(fd_stdout == 1);

	// char rdbuf[128];
	// // while(1){
	// // 	printl("C");
	// // 	milli_delay(2000);
	// // }
	// while (1) {
	// 	printf("[Ye's OS-TTY #2]-$ ");
	// 	int r = read(fd_stdin, rdbuf, 70);
	// 	rdbuf[r] = 0;

	// 	if (strcmp(rdbuf, "hello") == 0)
	// 		printf("hello world!\n");
	// 	else
	// 		if (rdbuf[0])
	// 			printf("{%s}\n", rdbuf);
	// }

	spin("TestC");
	assert(0); /* never arrive here */
}




/*****************************************************************************
 *                                panic:系统任务出错后停机，以函数形式写在main.c里
 * 			但是assert却是以宏的形式写在const.h里，应该是为了用户程序也可以用吧
 * 								panic()只能用在OS ring0、1下
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}


PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


