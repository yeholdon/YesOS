/*****************************************************
 * 程序名称：config.h
 * 程序功能：系统配置，比如系统分区等
 * 修改日期：2019.12.21
 */

#define	MINOR_BOOT			MINOR_hd2a

/*
 * disk log
 */
#define ENABLE_DISK_LOG
#define SET_LOG_SECT_SMAP_AT_STARTUP
#define MEMSET_LOG_SECTS
#define	NR_SECTS_FOR_LOG		NR_DEFAULT_FILE_SECTS