#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
#include "flash_drive.h"
#include "macro.h"
#include "mdfs_node.h"

/* global variable */
__u8 *flash_drive[TOTAL_BLK_NUM];

/* i/o wrapper functions */
int flash_read(loff_t phy, size_t len, size_t *retlen, __u8 *buf){
	__u8 *block;
	__u32 bn ,fofs, bofs = 0;
	size_t count = len;
	int fd;

	bn = phy >> BLOCK_SHIFT;
	fofs = phy % BLOCK_SIZE;
	while(count){
		block = flash_drive[bn];
		for(; count > 0 && fofs < BLOCK_SIZE; fofs++, bofs++, count--){
			buf[bofs] = block[fofs];
		}
		bn++;
		fofs = 0;
	}
	*retlen = len - count;
	return 0;
}

/*
 * write buffer into flash
 * phy : starting address on flash
 * len : buffer lenghth
 * retlen : if not null, record how many bytes writed on flash
 * buf : data buffer
 */
int flash_write(loff_t phy, size_t len, size_t *retlen, const __u8 *buf){
	__u8 *block;
	__u32 bn ,fofs, bofs = 0;
	size_t count = len;
	int fd;

	bn = phy >> BLOCK_SHIFT;
	fofs = phy % BLOCK_SIZE;

	while(count){
		block = (char *)flash_drive[bn];
		for(; count > 0 && fofs < BLOCK_SIZE; fofs++, bofs++, count--){
			block[fofs] = block[fofs] & buf[bofs];
		}
		bn++;
		fofs = 0;
	}
	
	if(retlen != NULL)
		*retlen = len - count;
	
	return 0;
}

int flash_dump(){
	char drive_name[20], block[BLOCK_SIZE];
	int fd, i;

	for(i = 0; i < TOTAL_BLK_NUM; i++){
		NAME_DRIVE(drive_name, i);
		fd = open(drive_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		write(fd, flash_drive[i], BLOCK_SIZE);
		close(fd);
	}

	return 0;
}

void init_drive(){
	int i;

	for(i = 0; i < TOTAL_BLK_NUM; i++){
		flash_drive[i] = kmalloc(BLOCK_SIZE, GFP_KERNEL);
		memset(flash_drive[i], 255, BLOCK_SIZE);
	}
}

void format_drive(){
	struct mdfs_raw_inode *mri = raw_inode_alloc();
	__u16 idx;
	int i;

	/* alloc the fist ptrptr for block list to connect next block */
	blk_init(HEAD_BLK_IDX, FH_NULL, FH_NULL);	
	/* anchor the raw inode "root" with MAX_LEVEL height */
	mri->pino = 0;
	mri->ino = 1;
	mri->dsize = 0;
	strcpy((char *)mri->name, "/");
	mri->nsize = strlen("/");
	mri->level = MAX_LEVEL;

	idx = blk_ctt_alloc(HEAD_BLK_IDX);
	for(i = 0; i < MAX_LEVEL ; i++){
		//mri->forward[i] = blk_ptr_bmp_ffs(0);
		//blk_ptr_bmp_set_zero(0, mri->forward[i]);
		mri->forward[i] = blk_ptr_alloc(HEAD_BLK_IDX);
	}

	for(i = 0; i < MAX_LEVEL; i++){
		ptrptr_update(HEAD_BLK_IDX, BLK_ANCHOR_IDX + i, idx);
	}
	
	raw_inode_write(HEAD_BLK_IDX, mri, idx);
	kfree(mri);
}
