/*****************************************************
 * 程序名称：fs.h
 * 程序功能：文件系统相关宏定义等
 * 修改日期：2019.12.21
 */

#ifndef _YE_FS_H_
#define _YE_FS_H_

/** 获取某类设备对应的驱动进程ID
 * @struct dev_drv_map fs.h "include/sys/fs.h"
 * @brief  The Device_nr.\ - Driver_nr.\ MAP.
 */
struct dev_drv_map {
	int driver_nr; /**< The proc nr.\ of the device driver. */
};

#endif