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

PRIVATE void init_fs();
PRIVATE void mkfs();

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
	for (i = 0; i < nr_sects / 8; i++)
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
}



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
