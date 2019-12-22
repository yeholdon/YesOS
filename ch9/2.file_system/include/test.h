/* macros for messages , IPC消息相关的宏定义*/
#define	FD		u.m3.m3i1
#define	PATHNAME	u.m3.m3p1
#define	FLAGS		u.m3.m3i1
#define	NAME_LEN	u.m3.m3i2
#define	CNT		u.m3.m3i2
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF		u.m3.m3p2
#define	OFFSET		u.m3.m3i2
#define	WHENCE		u.m3.m3i3

#define	RETVAL		u.m3.m3i1



/* Hard Drive，硬盘相关宏定义 */
#define SECTOR_SIZE		512
#define SECTOR_BITS		(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9
/* major device numbers (corresponding to kernel/global.c::dd_map[])
	主设备号，供dev_to_dri_map[]数组使用
 */
#define	NO_DEV			0
#define	DEV_FLOPPY		1
#define	DEV_CDROM		2
#define	DEV_HD			3
#define	DEV_CHAR_TTY		4
#define	DEV_SCSI		5
/* make device number from major and minor numbers 
个设备号都由主设备号和从设备号组成
所以这两个宏用来组合主从设备号为总设备号
*/
#define	MAJOR_SHIFT		8
#define	MAKE_DEV(a,b)		((a << MAJOR_SHIFT) | b)
/* separate major and minor numbers from device number 
后面用到的每个设备号都由主设备号和从设备号组成
所以这两个宏用来分离
*/
#define	MAJOR(x)		((x >> MAJOR_SHIFT) & 0xFF)
#define	MINOR(x)		(x & 0xFF)

/* device numbers of hard disk */
#define	MINOR_hd1a		0x10
#define	MINOR_hd2a		0x20
#define	MINOR_hd2b		0x21
#define	MINOR_hd3a		0x30
#define	MINOR_hd4a		0x40

#define	ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_BOOT)	/* 3, 0x21 */

#define	INVALID_INODE		0
#define	ROOT_INODE		1

#define	MAX_DRIVES		2										// 这里考虑硬盘接在主IDE通道的情况，所以最多两个硬盘
#define	NR_PART_PER_DRIVE	4						// 每个硬盘的主（扩展）分区数
#define	NR_SUB_PER_PART		16						// 每个扩展分区的最大逻辑分区数
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1)		// 等于5，这是因为hd0表示整个硬盘，hd1-4才表示4个主分区

/**
 * @def MAX_PRIM
 * Defines the max minor number of the primary partitions.
 * If there are 2 disks, prim_dev ranges in hd[0-9], this macro will
 * equals 9. 第一块硬盘的4个分区1-4,第二块的为6-9，MAX_PRIM为最大主分区序号为9
 * 作用是方便和它比较来区分是主分区还是逻辑分区
 */
#define	MAX_PRIM		(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)

#define	MAX_SUBPARTITIONS	(NR_SUB_PER_DRIVE * MAX_DRIVES)



/* device numbers of hard disk，次设备号 */
#define	MINOR_hd1a		0x10
#define	MINOR_hd2a		(MINOR_hd1a+NR_SUB_PER_PART)
// 指示主硬盘（OS所在硬盘）的总设备号
#define	ROOT_DEV		MAKE_DEV(DEV_HD, MINOR_BOOT)

#define	P_PRIMARY	0
#define	P_EXTENDED	1

// 分区类型
#define YES_PART	0x99	/* Ye'S OS partition */
#define NO_PART		0x00	/* unused entry */
#define EXT_PART	0x05	/* extended partition */

#define	NR_FILES	64
#define	NR_FILE_DESC	64	/* FIXME */
#define	NR_INODE	64	/* FIXME */
#define	NR_SUPER_BLOCK	8