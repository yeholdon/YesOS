/**************************************************************
 * main.c
 * 程序功能：新建了一个文件夹fs用来放文件系统模块，因为文件系统是很重要的一部分
 * 修改日期：2019.12.18
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "config.h"

#include "hd.h"
#include "stdio.h"

PRIVATE void init_fs();
PRIVATE void mkfs();
PRIVATE void read_super_block(int dev);
/*****************************************************************************
 *                                task_fs:文件系统任务，这里充当那个请求硬盘IO的用户进程的作用
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK FS.
 * 
 *****************************************************************************/
PUBLIC void task_fs()
{
	printl("Task FS begins.\n");

	// /* open the device: hard disk */ 转到init_fs()函数中
	// MESSAGE driver_msg;
	// driver_msg.type = DEV_OPEN;
	// // printl("%d\n", ROOT_DEV);
	// driver_msg.DEVICE = (int) MINOR(ROOT_DEV);			// 启动设备号，由主设备号3也就是代表从硬盘启动，从设备号16（第二个分区的第一个逻辑分区）两部分
	// 																									// 组成，代表从该分区启动这个分区启动
	// // printf("%d", driver_msg.DEVICE);
	// assert(dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
    // // 发请求读硬盘数据消息，并等待直到收到可读数据的消息
	// send_recv(BOTH, dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
	init_fs();


	while (1) {
		send_recv(RECEIVE, ANY, &fs_msg);

		int src = fs_msg.source;
		pcaller = &proc_table[src];

		switch (fs_msg.type) {
		case OPEN:
			fs_msg.FD = do_open();
			break;
		case CLOSE:
			fs_msg.RETVAL = do_close();
			break;
		case READ: 
		case WRITE: 
			fs_msg.CNT = do_rdwt();
		 	break; 
		/* case LSEEK: */
		/* 	fs_msg.OFFSET = do_lseek(); */
		/* 	break; */
		/* case UNLINK: */
		/* 	fs_msg.RETVAL = do_unlink(); */
		/* 	break; */
		/* case RESUME_PROC: */
		/* 	src = fs_msg.PROC_NR; */
		/* 	break; */
		/* case FORK: */
		/* 	fs_msg.RETVAL = fs_fork(); */
		/* 	break; */
		/* case EXIT: */
		/* 	fs_msg.RETVAL = fs_exit(); */
		/* 	break; */
		/* case STAT: */
		/* 	fs_msg.RETVAL = do_stat(); */
		/* 	break; */
		default:
			dump_msg("FS::unknown message:", &fs_msg);
			// assert(0);
			break;
		}

		/* reply */
		fs_msg.type = SYSCALL_RET;
		send_recv(SEND, src, &fs_msg);
	}
	spin("FS");
}


/*****************************************************************************
 *                                init_fs
 *****************************************************************************/
/**
 * <Ring 1> Do some preparation.
 * 
 *****************************************************************************/
PRIVATE void init_fs()
{

	int i;
	// 初始化几个文件系统相关结构
	/* f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));

	/* inode_table[] */
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));

	/* super_block[] */
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NO_DEV;

	// 打开设备：硬盘
	/* open the device: hard disk */
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	// printl("%d\n", ROOT_DEV);
	driver_msg.DEVICE = (int) MINOR(ROOT_DEV);			// 启动设备号，由主设备号3也就是代表从硬盘启动，从设备号16（第二个分区的第一个逻辑分区）两部分
																										// 组成，代表从该分区启动这个分区启动
	// printf("%d", driver_msg.DEVICE);
	assert(dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
    // 发请求读硬盘数据消息，并等待直到收到可读数据的消息
	send_recv(BOTH, dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	mkfs();

	/* load super block of ROOT */
	// 读取ROOT目录所在硬盘各个分区的超级块
	read_super_block(ROOT_DEV);
	// 得到指定设备的超级块指针
	sb = get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
	// 得到第ROOT_INODE号ROOT_DEV设备的root_inode
	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
PRIVATE void mkfs()
{
	// 先给硬盘驱动发消息，读取根目录所在盘信息
	MESSAGE driver_msg;

	struct part_info geo;
	driver_msg.type = DEV_IOCTL;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	driver_msg.REQUEST = DIOCTL_GET_DEV_INFO;
	driver_msg.BUF = &geo;
	driver_msg.PROC_NR = TASK_FS;
	// 从操作的主设备号获取其对应的驱动号
	assert(dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	// 给设备（硬盘）对应的驱动程序（硬盘驱动）发消息读取该分区信息
	send_recv(BOTH, dev_to_dri_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
	// 读取的信息存在geo里
	printl("dev size: 0x%x sectors\n", geo.size);
	
	// 准备super block结构信息，并写入 硬盘分区的第一个扇区
	u32 bits_per_sector = SECTOR_SIZE * 8;
	struct super_block sb;
	sb.magic = MAGIC_V1;
	sb.nr_inodes = bits_per_sector;		// 4096个inode
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;
	sb.nr_sects = geo.size;
	sb.nr_imap_sects = 1;			// 因为imap只占一个扇区，所以总的inode数也只有bite_per_sector
	sb.nr_smap_sects = sb.nr_sects / bits_per_sector + 1; // 一个扇区一位
	sb.n_1st_sect = 1 + 1 +			// 分别是boot sector & super block
									sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
	sb.root_inode = ROOT_INODE;  // =1,0号inode保留，表示不正确的inode或者不存在inode
	sb.inode_size = INODE_SIZE;
	struct inode s;
	sb.inode_isize_off = (u32)&s.i_size - (u32)&s; // 以字节为步长
	sb.inode_start_off = (u32)&s.i_start_sect - (u32)&s; //第一个数据扇区
	sb.dir_ent_size = DIR_ENTRY_SIZE;
	struct dir_entry dir;
	sb.dir_ent_inode_off = (u32)&dir.inode_nr - (u32)&dir;
	sb.dir_ent_fname_off = (u32)&dir.name - (u32)&dir;

	// 先把超级块内容拷贝进fsbuf
	memset(fsbuf, 0x90, SECTOR_SIZE);	// super block占一个扇区
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);
	
	// 正式通过IPC将super block写入硬盘
	WR_SECT(ROOT_DEV, 1);
	
	// 打印文件系统几个结构要素的起始地址
	printl("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
		"        inodes:0x%x00, 1st_sector:0x%x00\n", 
		geo.base * 2,				// 因为一个扇区512字节，geo.base是以扇区为单位的，所以转成字节为单位
		(geo.base + 1) * 2,		// 要除512，即左移9位，打印的时候后面直接添了2个0，所以只要再左移一位即可
		(geo.base + 1 + 1) * 2,
		(geo.base + 1 + 1 + sb.nr_imap_sects) * 2,
		(geo.base + 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects) * 2,
		(geo.base + sb.n_1st_sect) * 2);
	
	// 下面设置inode map
	memset(fsbuf, 0, SECTOR_SIZE);	// 用缓冲区时都先初始化，用多少初始化多少
	for (int i = 0; i < (NR_CONSOLES + 2); i++)
	{
		// tty等也看成是文件来管理，万物皆文件是Linux的一大特点
		fsbuf[0] |= 1 << i;			// 把前NR_CONSOLES + 2 bit置1，表示创建了相应文件
	}
	assert(fsbuf[0] == 0x1F);/* 0001 1111 : 
				  *    | ||||
				  *    | |||`--- bit 0 : reserved
				  *    | ||`---- bit 1 : the first inode,
				  *    | ||              which indicates `/' 根目录放inode
				  *    | |`----- bit 2 : /dev_tty0
				  *    | `------ bit 3 : /dev_tty1
				  *    `-------- bit 4 : /dev_tty2
				  */
	
	WR_SECT(ROOT_DEV, 1 + sb.nr_imap_sects );	// 第2扇区是inode map

	// sector map
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	/*             ~~~~~~~~~~~~~~~~~~~|~   |
	 *                                |    `--- bit 0 is reserved
	 *                                `-------- for `/'
	 */
	int i;
	for (i = 0; i < nr_sects >> 3; i++)
		fsbuf[i] = 0xFF;

	for (int j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(ROOT_DEV, 1 + sb.nr_imap_sects + 1);

	/* zeromemory the rest sector-map */
	memset(fsbuf, 0, SECTOR_SIZE);
	for (int i = 1; i < sb.nr_smap_sects; i++)
		WR_SECT(ROOT_DEV, 1 + sb.nr_imap_sects + 1 + i);
	

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;	// 以inode结构为步长，所以转成相应类型的指针方便设置
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4; /* 4 files:
					  * `.', 所以根目录的大小是包含了其下所有文件的大小之和的
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;	// 扁平模型，就一个/所以前面开的总的使用的扇区数就是它
	/* inode of `/dev_tty0~2' */
	for (int i = 0; i < NR_CONSOLES; i++) {
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 1 + sb.nr_imap_sects + sb.nr_smap_sects + 1);

	// 文件本身
	// 首先是特殊的文件——根目录文件
	// dir_entry结构是存在于根目录文件中的，用来索引一个文件，有几个文件根目录文件里就有几个dir_entry项
	/*          `/'        首先是根目录文件 */
	// 根目录文件的第一个dir_entry
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = 1;		// 根目录的inode序号为1
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		sprintf(pde->name, "dev_tty%d", i);	// 可见文件名是不带/号的
	}
	WR_SECT(ROOT_DEV, sb.n_1st_sect);	
}


int c = 0;
/*****************************************************************************
 *                                rw_sector:IPC发消息给驱动，读/写一个扇区
 *****************************************************************************/
/**
 * <Ring 1> R/W a sector via messaging with the corresponding driver.
 * 
 * @param io_type  DEV_READ or DEV_WRITE
 * @param dev      device nr
 * @param pos      Byte offset from/to where to r/w.
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs.
 * @param buf      r/w buffer.
 * 
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;

	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= pos;
	driver_msg.BUF		= buf;
	driver_msg.CNT		= bytes;
	driver_msg.PROC_NR	= proc_nr;

	assert(dev_to_dri_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);

	send_recv(BOTH, dev_to_dri_map[MAJOR(dev)].driver_nr, &driver_msg);

	return 0;
}




/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 * 从硬盘读取超级块并填入super_block[]的一个空闲项中
 * @param dev  From which device the super block comes.
 *****************************************************************************/
PRIVATE void read_super_block(int dev)
{
	int i;
	MESSAGE driver_msg;

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= fsbuf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= TASK_FS;
	assert(dev_to_dri_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dev_to_dri_map[MAJOR(dev)].driver_nr, &driver_msg);

	/* find a free slot in super_block[] */
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		panic("super_block slots used up");

	assert(i == 0); /* currently we use only the 1st slot */

	struct super_block * psb = (struct super_block *)fsbuf;

	super_block[i] = *psb;
	super_block[i].sb_dev = dev;
}


/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 * 获取设备dev的超级块
 * @param dev Device nr.
 * 
 * @return Super block ptr.
 *****************************************************************************/
PUBLIC struct super_block * get_super_block(int dev)
{
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		if (sb->sb_dev == dev)
			return sb;

	panic("super block of devie %d not found.\n", dev);

	return 0;
}


/*****************************************************************************
 *                                get_inode
 *****************************************************************************/
/**
 * <Ring 1> Get the inode ptr of given inode nr. A cache -- inode_table[] -- is
 * maintained to make things faster. If the inode requested is already there,
 * just return it. Otherwise the inode will be read from the disk.
 * 从dev硬盘读num号inode
 * @param dev Device nr.
 * @param num I-node nr.
 * 
 * @return The inode ptr requested.
 *****************************************************************************/
PUBLIC struct inode * get_inode(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode * p;
	struct inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->i_cnt) {	/* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num)) {
				/* this is the inode we want */
				p->i_cnt++;
				return p;
			}
		}
		else {		/* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block * sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects +
		((num - 1) / (u32)(SECTOR_SIZE / INODE_SIZE));	// 这里加了一个u32就好了！！！
	RD_SECT(dev, blk_nr);
	struct inode * pinode =
		(struct inode*)((u8*)fsbuf +
				((num - 1 ) % (SECTOR_SIZE / INODE_SIZE))
				 * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}

/*****************************************************************************
 *                                put_inode
 *****************************************************************************/
/**
 * Decrease the reference nr of a slot in inode_table[]. When the nr reaches
 * zero, it means the inode is not used any more and can be overwritten by
 * a new inode.
 * 
 * @param pinode I-node ptr.
 *****************************************************************************/
PUBLIC void put_inode(struct inode * pinode)
{
	assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

/*****************************************************************************
 *                                sync_inode
 *****************************************************************************/
/**
 * <Ring 1> Write the inode back to the disk. Commonly invoked as soon as the
 *          inode is changed.
 * 
 * @param p I-node ptr.
 *****************************************************************************/
PUBLIC void sync_inode(struct inode * p)
{
	struct inode * pinode;
	struct super_block * sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects +
		((p->i_num - 1) / (u32)(SECTOR_SIZE / INODE_SIZE));
	RD_SECT(p->i_dev, blk_nr);
	pinode = (struct inode*)((u8*)fsbuf +
				 (((p->i_num - 1) % (u32)(SECTOR_SIZE / INODE_SIZE))
				  * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr);

}