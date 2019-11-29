/*****************************************************
 * protect.h
 * 程序功能：段描述符结构体的typedef
 * 修改日期：2019.11.28
 */

#ifndef	_YE_PROTECT_H_
#define	_YE_PROTECT_H_

/* 存储段描述符/系统段描述符 ,按照定义的顺序*/
typedef struct s_descriptor		/* 共 8 个字节 */
{
	u16	limit_low;		/* Limit */
	u16	base_low;		/* Base */
	u8	base_mid;		/* Base */
	u8	attr1;			/* P(1) DPL(2) DT(1) TYPE(4) */
	u8	limit_high_attr2;	/* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
	u8	base_high;		/* Base */
} DESCRIPTOR;

#endif /* _YE_PROTECT_H_ */