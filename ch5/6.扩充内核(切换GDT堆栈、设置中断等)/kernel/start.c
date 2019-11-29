/**************************************************************
 * start.c
 * 程序功能：将loader中的GDT复制进kernel，并更新gdtr
 * 修改日期：2019.11.28
 */
#include "type.h"
#include "const.h"
#include "protect.h"

//要调用的两个asm函数的声明,挪动到统一存放声明的头文件proto.h和string.h中
//PUBLIC  void*   memcpy(void* pDst, void*  pSrc, int iSize);     //涉及到指针的，用void具体再强转
//PUBLIC void disp_str(char *pszInfo);

PUBLIC  u8  gdt_ptr[6];	/* 0~15:Limit  16~47:Base */
PUBLIC  DESCRIPTOR  gdt[GDT_SIZE];

PUBLIC  void cstart()
{

	/*复制gdt到内核，再把gdt_ptr里的内容更新指向内核中的gdt*/
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 			  "-----\"cstart\" begins-----\n");
	
    // 复制GDT, memcpy函数放在单独的string.asm文件里，和其它一些字符串相关操作放一起
    memcpy(&gdt,    // 目的地址
			(void *)(*((u32 *)(&gdt_ptr[2]))), //源地址,最后传进去的其实就是存在gdt[]里的GDT基地址
			*((u16*)(&gdt_ptr[0])) + 1				//这个也一样，只不过这个少了最后一步转成void *指针类型
    );

	// 更新gdt_ptr的内容:先获取指针再借助指针修改
	/* gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt/lgdt 的参数。*/
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base  = (u32)&gdt;

	disp_str("-----\"cstart\" ends-----\n");
}

