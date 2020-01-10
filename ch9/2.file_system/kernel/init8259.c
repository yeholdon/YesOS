/*****************************************************************************
 * init8259.c
 * 程序功能：初始化设置8259A中断控制接口芯片
 * 修改日期：2019.11.29
 */


#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "global.h"
#include "stdio.h"

/*======================================================================*
                            init_8259A
 *======================================================================*/
PUBLIC void init_8259A()
{
	/* Master 8259, ICW1. */
	out_byte(INT_M_CTL,	0x11);

	/* Slave  8259, ICW1. */
	out_byte(INT_S_CTL,	0x11);

	/* Master 8259, ICW2. 设置 '主8259' 的中断入口地址为 0x20. */
	out_byte(INT_M_CTLMASK,	INT_VECTOR_IRQ0);

	/* Slave  8259, ICW2. 设置 '从8259' 的中断入口地址为 0x28 */
	out_byte(INT_S_CTLMASK,	INT_VECTOR_IRQ8);

	/* Master 8259, ICW3. IR2 对应 '从8259'. */
	out_byte(INT_M_CTLMASK,	0x4);

	/* Slave  8259, ICW3. 对应 '主8259' 的 IR2. */
	out_byte(INT_S_CTLMASK,	0x2);

	/* Master 8259, ICW4. */
	out_byte(INT_M_CTLMASK,	0x1);

	/* Slave  8259, ICW4. */
	out_byte(INT_S_CTLMASK,	0x1);

	/* Master 8259, OCW1.  */
	// out_byte(INT_M_CTLMASK,	0xFE);		// 之前0xFF改成0xFD，打开了键盘中断，现在打开时钟中断
	out_byte(INT_M_CTLMASK,	0xFF);  // 现在可以通过enable_irq和disable_irq来手动开启和关闭了，那么初始化就先全部屏蔽掉，用到时再开启
	/* Slave  8259, OCW1.  */
	out_byte(INT_S_CTLMASK,	0xFF);		// 之前0xFF改成0xFD，打开了键盘中断，现在关掉

	// 初始化irq_table也就是外部硬件中断的中断处理函数的函数指针表的表项
	for (int i = 0; i < NR_IRQ; i++)
	{
		// 先都初始化为spurious_irq，就像一般都先初始化为空值一样，spurious_irq基本啥事都不做，只是为了中断功能是否实现
		irq_table[i] = spurious_irq;
	}
	

}

/*======================================================================*
                            put_irq_handler(int irq, irq_handler handler)
							专门用来填各个中断的中断函数的地址, 在kernel_main里调用
 *======================================================================*/
PUBLIC	void put_irq_handler(int irq, irq_handler handler)
{
	// 修改前先把中断禁用了，防止改到一半中断发生而出错
	disable_irq(irq); //这个函数也封装成一个库函数，方便对各个中断进行禁用，写在klib.asm里，毕竟要操作寄存器
	irq_table[irq] = handler;
}


PUBLIC	void spurious_irq(int irq) 
{
	disp_str("spurious_irq: ");
	disp_int(irq);
	disp_str("\n");
}