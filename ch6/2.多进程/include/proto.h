/**************************************************************
 * proto.h
 * 程序功能：存放各个主要函数的声明
 * 修改日期：2019.11.28
 */

// klib.asm
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void disp_int(int input);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

// protect.c
PUBLIC void	init_prot();
PUBLIC void	init_8259A();
PUBLIC u32	seg2phys(u16 seg);

// klib.c 
PUBLIC void	delay(int time);

// kernel.asm 
void restart();

// main.c 
void TestA();