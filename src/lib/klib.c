/*****************************************************
 * klib.c
 * 程序功能：进制转换和显示等额外功能函数
 * 修改日期：2019.11.30
 */

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "global.h"
#include  "config.h"
#include "elf.h"
/*======================================================================*
                               itoa:把int转成一位位的16进制字符显示出来，相比C标准库里的简单许多
 *======================================================================*/
/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */

PUBLIC char *itoa(char *str, int num) 
{
    char *p = str;
    char ch;
    int i;
    int flag = 0;

    *p++ = '0';
    *p++ = 'x';

    if(num == 0) {
        *p++ = '0';
    }
    else {
        for(i = 28; i >= 0; i -= 4) {
            ch = (num >> i) & 0xF;
            if(flag  || ch > 0) {       // 前面的0不显示
                flag = 1;
                ch += '0';
                if(ch > '9') {
                    ch += 7;
                }
                *p++ = ch;
            }
        }
    }
    *p = 0;     //最后补的这个0好像没啥意义，就是一个结束符
    return str;
}

/*======================================================================*
                               disp_int
 *======================================================================*/
PUBLIC void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}


/*======================================================================*
                               delay()：延时函数
 *======================================================================*/
PUBLIC  void delay(int time) 
{
    for (int k = 0; k < time; k++)
    {
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 10000; j++)
            {
                    
            }
            
        }
        
    }
    
}


int max(int a, int b) {
    if (a > b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

int min(int a, int b) {
    if (a < b)
    {
        return a;
    }
    else
    {
        return b;
    }  
}

// 获取loader预先保存到内存中的参数
/*****************************************************************************
 *                                get_boot_params
 *****************************************************************************/
/**
 * <Ring 0~1> The boot parameters have been saved by LOADER.
 *            We just read them out.
 * 
 * @param pbp  Ptr to the boot params structure
 *****************************************************************************/
PUBLIC void get_boot_params(struct boot_params * pbp)
{
	/**
	 * Boot params should have been saved at BOOT_PARAM_ADDR.
	 */
	int * p = (int*)BOOT_PARAM_ADDR;
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	pbp->mem_size = p[BI_MEM_SIZE];
	pbp->kernel_file = (unsigned char *)(p[BI_KERNEL_FILE]);

	/**
	 * the kernel file should be a ELF executable,
	 * check it's magic number 这两个宏来自/usr/include/elf.h
	 */
	assert(memcmp(pbp->kernel_file, ELFMAG, SELFMAG) == 0);
}


/*****************************************************************************
 *                                get_kernel_map:获取内核在内存中的内存范围
 *****************************************************************************/
/**
 * <Ring 0~1> Parse the kernel file, get the memory range of the kernel image.
 * 这里占用的是0x1000~0x3BFA8约240KB
 * - The meaning of `base':	base => first_valid_byte
 * - The meaning of `limit':	base + limit => last_valid_byte
 * 
 * @param b   Memory base of kernel.
 * @param l   Memory limit of kernel.
 *****************************************************************************/
PUBLIC int get_kernel_map(unsigned int * b, unsigned int * l)
{
	struct boot_params bp;
	get_boot_params(&bp);

	Elf32_Ehdr* elf_header = (Elf32_Ehdr*)(bp.kernel_file);

	/* the kernel file should be in ELF format */
	if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0)
		return -1;

	*b = ~0;
	unsigned int t = 0;
	int i;
	for (i = 0; i < elf_header->e_shnum; i++) {
		Elf32_Shdr* section_header =
			(Elf32_Shdr*)(bp.kernel_file +
				      elf_header->e_shoff +
				      i * elf_header->e_shentsize);

		if (section_header->sh_flags & SHF_ALLOC) {
			int bottom = section_header->sh_addr;
			int top = section_header->sh_addr +
				section_header->sh_size;

			if (*b > bottom)
				*b = bottom;
			if (t < top)
				t = top;
		}
	}
	assert(*b < t);
	*l = t - *b - 1;

	return 0;
}
