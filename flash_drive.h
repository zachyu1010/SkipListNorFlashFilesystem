#ifndef _FLASH_DRIVE_H_
#define _FLASH_DRIVE_H_

#define __u32 unsigned int
#define __u16 unsigned short
#define __u8 unsigned char


/* constant */
#define DRIVE_SIZE		128*1024*1024		/*4-mega drive*/
#define BLOCK_SHIFT		17
#define BLOCK_SIZE		(1UL << BLOCK_SHIFT)
#define TOTAL_BLK_NUM	32//(DRIVE_SIZE >> BLOCK_SHIFT)
#define BLK_PRNT_IDX	0
#define BLK_NXT_IDX		1		/* block list index */
#define BLK_ANCHOR_IDX	2		/* anchored node index */
#define TEMP_BLK_IDX	0		/* preserve for pointer scrub*/
#define M1_INIT_IDX		1
#define M2_INIT_IDX		2
#define HEAD_BLK_IDX	3		/* block index of root "\" in the initial */
#define BLK_PRSRV_NUM	3
#define BLK_RSRV_SPC	(BLK_INODE_NUM * 0.15)

/* merge/split-policy constant */
#define NONE_VALID_PER_CHIP	1
#define STABLE_SCALE	BLK_INODE_NUM
#define AGE_FACTOR		12
//#define AGE_WRI_NUM		(BLK_INODE_NUM * AGE_FACTOR)
#define AGE_WRI_NUM		(1000 * AGE_FACTOR)

/* block type */
#define BLK_TYPE_FREE	0
#define BLK_TYPE_PRSRV	1
#define BLK_TYPE_META	2
#define BLK_TYPE_DATA	3

/* flash_drive operations */
extern void init_drive(); 
extern int flash_read(loff_t phy, size_t len, size_t *retlen, __u8 *buf);
extern int flash_write(loff_t phy, size_t len, size_t *retlen, const __u8 *buf);
extern int flash_dump();
	
/* macro function */
#define NAME_DRIVE(buf, i)		sprintf(buf, "drive/%04d.drv", i);

/* flash layout parameter */

/* area size */
#define BLK_ATTR_SIZE	2
#define BLK_CTT_BMP_ALLOT_SIZE 	170	/* size of block's content bitmap for allotment */
#define BLK_CTT_BMP_STATE_SIZE	170	/* size of block's content bitmap for state*/
#define BLK_CTT_SIZE		108800	/* size of block's content */
#define BLK_PTR_BMP_SIZE	306		/* size of block's pointer bitmap */
#define BLK_PTR_SIZE		21248	/* size of block's pointer */

/* start address of areas*/
#define BLK_ATTR_OFFSET	0
#define BLK_CTT_BMP_OFFSET	(BLK_ATTR_OFFSET + BLK_ATTR_SIZE)	/* start address of block's bitmap */
#define BLK_CTT_BMP_ALLOT_OFFSET	BLK_CTT_BMP_OFFSET
#define BLK_CTT_BMP_STATE_OFFSET	(BLK_CTT_BMP_ALLOT_OFFSET + BLK_CTT_BMP_ALLOT_SIZE)
#define BLK_CTT_OFFSET		(BLK_CTT_BMP_STATE_OFFSET + BLK_CTT_BMP_STATE_SIZE)
#define BLK_PTR_BMP_OFFSET	(BLK_CTT_OFFSET + BLK_CTT_SIZE) 	/* start address of pointer bitmap */
#define BLK_PTR_OFFSET		(BLK_PTR_BMP_OFFSET + BLK_PTR_BMP_SIZE) 	/* start address of object pointer */
#define BLK_END			(BLK_PTR_OFFSET + BLK_PTR_SIZE) 	/* the end of the block */

/* number of flash component*/
#define BLK_INODE_NUM		(BLK_CTT_BMP_ALLOT_SIZE << 3)
#define BLK_PTR_NUM			(BLK_PTR_BMP_SIZE << 3)
#define TOTAL_INODE_NUM	(BLK_INODE_NUM * TOTAL_BLK_NUM)

/* spacial value on nor flash */
#define FH_NULL 0xFFFF		/* NULL */
#define FH_XPRT 0x8000		/* exporting pointer*/
#define FH_DIRTY 0x0000


/* global variable */
extern __u8 *flash_drive[TOTAL_BLK_NUM];

#endif
