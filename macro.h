#ifndef _MACRO_H_
#define _MACRO_H_

#define printk(format, arg...)		printf(format, ##arg)
#define kfree(ptr) 	free(ptr)
#define kmalloc(size, GFP_KERNEL)	malloc(size)
#define KERN_INFO       ""   /* kernel informational */

#define __u32 unsigned int
#define __u16 unsigned short
#define __u8 unsigned char


#endif
