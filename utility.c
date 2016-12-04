#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include "flash_drive.h"
#include "mdfs_node.h"
#include "utility.h"
#include "macro.h"

#define MODE(size ,mod) ((mod-1) & (size))

__u8 flash_blk_bmp[TOTAL_BLK_NUM >> 3];

/* miscellaneous operations */
/*
 * dump the memory with the required length
 */
void memdump(__u8 *buf, int len){
	int i;
//	printk(KERN_INFO "memlen = %d\n", len);
	for(i = 0; i < len; i++){
		if(i != 0 && (i % 16) == 0)
			printk(KERN_INFO "\n");
		printk(KERN_INFO "%02X", buf[i]);
	}
	printk(KERN_INFO "\n");
}

/*
 * check if the skip list pointers are correct on the flash
 */
int fsck(struct sb_info *sb){
	__u16 cut[MAX_LEVEL], /* record the index of blocks in the cut of the skip list*/
		idx, cb, nb, k;
	struct mdfs_raw_inode x, tx;
	int i, err = 0;
	__u32 total = 0, count = 1;
	
	cb = sb->blk_idx;
	nb = blk_next_get(cb);
	raw_inode_read(cb, &x, ptrptr_cur_read(cb, BLK_ANCHOR_IDX, NULL));
	for(i = 0; i < MAX_LEVEL; i++){
		cut[i] = ptrptr_cur_read(cb, x.forward[i], NULL);
	}
	printk(KERN_INFO "\t\t==== filesystem ckeck ==== \t\t\n");
	printk(KERN_INFO "fsck begin...\n");
	k = cut[0];
	while(1){
		if(k == FH_NULL)
			break;

		if(is_ptr_export(k)){ /* exporting pointer */
			for(i = 0; i < MAX_LEVEL; i++){
				if(cut[i] != FH_XPRT){
					printk(KERN_INFO "fsck: exporting pointer error in block %d\n", cb);
					err = 1;
					goto out;
				}
			}

			if(count != blk_obj_num_get(cb)){
				printk(KERN_INFO "fsck: block %d has lost some nodes\n", cb);
				err = 1;
				goto out;
			}
			count = 0;

			cb = nb;
			nb = blk_next_get(cb);
			k = ptrptr_cur_read(cb, BLK_ANCHOR_IDX, NULL);
			for(i = 0; i < MAX_LEVEL; i++){
				cut[i] = ptrptr_cur_read(cb, BLK_ANCHOR_IDX + i, NULL);
			}
		}
		
		tx = x;
		raw_inode_read(cb, &x, k);
		count++;
		total++;
//		if((strcmp(tx.name, "/") != 0) && (raw_inode_cmp(tx.pino, tx.name, x.pino, x.name) >= 0)){
		if(raw_inode_cmp(tx.pino, tx.name, x.pino, x.name) >= 0){
			printk("fsck: premutaion error...\n");
			printk("fsck: (%u, %s) before (%u, %s)\n", tx.pino, tx.name, x.pino, x.name);
			err = 1;
			goto out;
		}

		for(i = 0; i < x.level; i++){
			/* cut[0]~cut[mk] must point to the next raw inode "x" */

			if(cut[i] != k){
				printk(KERN_INFO "(%u, %u, %s) miss pointers\n", tx.pino, tx.ino, tx.name);
				err = 1;
				goto out;
			}
			cut[i] = ptrptr_cur_read(cb, x.forward[i], NULL);
		}
		k = cut[0];
	}

	if(total != state_nr.ctt_valid_cnt){
		printk(KERN_INFO "fsck: some node lost...\n");
		err = 1;
	}
out:		
	printk(KERN_INFO "fsck complete...\n");
	return err;
}

/* bitmap operations */
/*
 * Find the fist set bit of the given buffer
 * The index of the first bit set numbered from 0, or 0 if can't finding.
 */
static __u32 __find_first_bit(char *__buf, __u32 __len){
	int *buf = (int *)__buf;
	__u32 count = __len >> 5;
	int k = MODE(__len >> 3, 4);
	int i, j, x, rem;

	for(i = j = x = 0; i < count; i++, j += (4 << 3)){
		x = ffs(buf[i]);
		if(x)
			break;
	}
	/*count the bytes not align to 4*/
	if((i == count) && k){
		__u8 *tb = (__u8 *)&buf[count];

		for(rem = 0; k > 0; k--){
			rem = rem << 8;
			rem += tb[k - 1];
		}

		x = ffs(rem);
	}
	return (!x) ? x : j + x;
}

/*
 * Set the idxth bit to one of the given buffer
 * the index number is numbered from one
 */
static void __set_bit_one(char *__buf, __u32 idx){
	char *buf;
	__u8 value;
	size_t retlen;

	if(idx == 0)
		return;
	
	buf = __buf + ((idx - 1) >> 3);
	value = (1UL << MODE(idx - 1, 8));
	*buf = (*buf) | value;
}

/*
 * Set the idxth bit to zero of the given buffer
 * the index number is numbered from one
 */
static void __set_bit_zero(char *__buf, __u32 idx){
	char *buf;
	__u8 value;
	size_t retlen;

	if(idx == 0)
		return;
	
	buf = __buf + ((idx - 1) >> 3);
	value = ~(1UL << MODE(idx - 1, 8));

	*buf = (*buf) & value;
}

/*
 * Find the first bit set of the block's contnet bitmap for free space.
 * The index of the first bit set numbered from 0, or FH_NULL if can't finding.
 */
__u32 blk_ctt_bmp_ffs(__u32 blk){
	char cbmp[BLK_CTT_BMP_ALLOT_SIZE];
	loff_t phy = (blk << BLOCK_SHIFT) + BLK_CTT_BMP_ALLOT_OFFSET;
	size_t retlen;
	int idx;

	flash_read(phy, BLK_CTT_BMP_ALLOT_SIZE, &retlen, (__u8 *)cbmp);
	idx = __find_first_bit(cbmp, BLK_INODE_NUM);
	return idx ? idx - 1 : FH_NULL;
}

/*
 * set the idxth bit of the block content bitmap to zero
 * The idx start from 0
 */
void blk_ctt_bmp_set_zero(__u32 blk, __u32 idx){
	loff_t phy;
	__u8 value;
	size_t retlen;

	if(idx >= BLK_INODE_NUM){
		printk(KERN_INFO "blk_ctt_bmp_set_zero: the param idx is too large index\n");
		return;
	}
	phy = (blk << BLOCK_SHIFT) + BLK_CTT_BMP_ALLOT_OFFSET + (idx >> 3);
	value = ~(1UL <<  MODE(idx, 8));
	flash_write(phy, 1, &retlen, &value);
}

/*
 * Find the first bit set of the block's pointer bitmap for free space.
 * The index of the first bit set numbered from 0, or FH_NULL if can't finding.
 */
__u32 blk_ptr_bmp_ffs(__u32 blk){
	char pbmp[BLK_PTR_BMP_SIZE];
	loff_t phy = (blk << BLOCK_SHIFT) + BLK_PTR_BMP_OFFSET;
	size_t retlen;
	int idx;

	flash_read(phy, BLK_PTR_BMP_SIZE, &retlen, (__u8 *)pbmp);
	idx = __find_first_bit(pbmp, BLK_PTR_NUM);
	return idx ? idx - 1 : FH_NULL;
}

/*
 * set the idxth bit of the block content bitmap to zero
 * The idx start from 0
 */
void blk_ptr_bmp_set_zero(__u32 blk, __u32 idx){
	loff_t phy;
	__u8 value;
	size_t retlen;

	if(idx >= BLK_PTR_NUM){
		printk(KERN_INFO "blk_ptr_bmp_set_zero: the param idx is too large index\n");
		return;
	}
	phy = (blk << BLOCK_SHIFT) + BLK_PTR_BMP_OFFSET + (idx >> 3);
	value = ~(1UL <<  MODE(idx, 8));
	flash_write(phy, 1, &retlen, &value);
}

/* pointer operations */

/*
 * return the last block index by walking through the linking path starting from param idx.
 * and the param last directing the last ptr index for short cut.
 * blk: the searching block index
 * idx: the index any where on the pointer update list
 * last: the last index of the pointer
 : return the index of the next raw inode
 */
__u16 ptr_follow(__u16 blk, __u16 idx, __u16 *last){
	loff_t phy, pos;
	size_t retlen;
	struct ptrptr ptrptr;

	phy = (blk << BLOCK_SHIFT) + BLK_PTR_OFFSET;
	while(1){
		state_nr.ptr_read_cnt++;
		pos = phy + (idx * sizeof(struct ptrptr));
		flash_read(pos, sizeof(struct ptrptr), &retlen, (__u8 *)&ptrptr);
		if(ptrptr.next == 0xFFFF){
			if(last != NULL)
				*last = idx;
			return ptrptr.cur;
		}
		idx = ptrptr.next;
	}
}

/* write the new object pointer into pointer area */
__u16 ptrptr_write(__u32 blk, struct ptrptr *ptrptr){
	__u8 *buf = (unsigned char *)ptrptr;
	__u16 idx;
	size_t retlen;
	loff_t phy;
	
	state_nr.ptr_write_cnt++;
	idx = blk_ptr_alloc(blk);
	phy = (blk << BLOCK_SHIFT) + BLK_PTR_OFFSET + (idx * sizeof(struct ptrptr));
	flash_write(phy, sizeof(struct ptrptr), &retlen, buf);
	return idx;
}

/*
 * return the next index of the block, and ptr index is stored in last for return
 */
__u16 ptrptr_cur_read(__u16 blk, __u16 idx, __u16 *last){
	return ptr_follow(blk, idx, last);
}

/*
 * throught the ptr link, add new ptr by mark old ptr for linking the new one
 * return the index of the new ptr instance.
 * the "last" parameter can be any index of the whole linking path.
 * blk: the block to be inserted
 * last: the any where on the pointer update list
 * idx: the index of next raw node
 * return the index of new pointer
 */

__u16 ptrptr_update(__u16 blk, __u16 last, __u16 cur){
	loff_t phy, pos;
	size_t retlen;
	__u16 k, ret;
	struct ptrptr ptrptr, update = {.cur = cur, .next = 0xFFFF};

	if(last == FH_NULL)
		return FH_NULL;

	phy = (blk << BLOCK_SHIFT) + BLK_PTR_OFFSET;
	/*
	 * NOTE: k maybe can get from caller and if wanting to reduce reading times, 
	 *        make the k parameter.
	 */
	k = ptr_follow(blk, last, &last);
	ret = last;
	/* If k == FH_NULL, the pointer must be .cur=0xFFFF .next=0xFFFF.          */
	/* So directly writing the new value into the ptrptr poiner without linking*/
	if(k == FH_NULL) 
		goto not_link;
	pos = phy + (last * sizeof(struct ptrptr));
//	ret = blk_ptr_bmp_ffs(blk);
//	blk_ptr_bmp_set_zero(blk, ret);
	ret = blk_ptr_alloc(blk);
	ptrptr.cur = 0x0000;
	ptrptr.next = ret;
	/* write into flash to mark the old ptr for linking the update ptr */
	flash_write(pos, sizeof(struct ptrptr), &retlen, (__u8 *)&ptrptr);
	
not_link:
	/* write the update ptr into flash for linking next object */
	pos = phy + (ret * sizeof(struct ptrptr));
	flash_write(pos, sizeof(struct ptrptr), &retlen, (__u8 *)&update);
	return ret;
}

/* inode operations */

/*
 * read a inode instance from flash corresponding the the idxth index
 */
int raw_inode_read(__u16 blk, struct mdfs_raw_inode *mri, __u16 idx){
	__u8 *buf = (__u8 *)mri;
	size_t retlen;
	loff_t phy;

	state_nr.ctt_read_cnt++;
	phy = (blk << BLOCK_SHIFT) + BLK_CTT_OFFSET + (idx * sizeof(struct mdfs_raw_inode));
	flash_read(phy, sizeof(struct mdfs_raw_inode), &retlen, buf);
	return 0;
}

/* 
 * write raw inode into flash corresponding to the idxth index
 */
int raw_inode_write(__u16 blk, struct mdfs_raw_inode *mri, __u16 idx){
	__u8 *buf = (unsigned char *)mri;
	size_t retlen;
	loff_t phy;

	state_nr.ctt_write_cnt++;
	idx = clear_ptr_export(idx); /* clear the exporting bit */
	phy = (blk << BLOCK_SHIFT) + BLK_CTT_OFFSET + (idx * sizeof(struct mdfs_raw_inode));
	flash_write(phy, sizeof(struct mdfs_raw_inode), &retlen, buf);
	return 0;
}


/* block operations */
/*
 * Check if the block blk has num free pointer objects
 * return 1, if having, otherwise 0.
 */
int blk_ptr_spc_chk(__u16 blk, __u16 num){
	__u16 c, idx = blk_ptr_bmp_ffs(blk);

	if(idx == FH_NULL)
		return 0;
	
	c = BLK_PTR_NUM - blk_ptr_bmp_ffs(blk);

	return (c >= num);
}

/*
 * Check if the block blk has num free ctt objects
 * return 1, if having, otherwise 0.
 */
int blk_ctt_spc_chk(__u16 blk, __u16 num){
	__u16 c, idx = blk_ctt_bmp_ffs(blk);

	if(idx == FH_NULL)
		return 0;

	c = BLK_INODE_NUM - idx;
	
	return (c >= num);
}

int free_blk_chk(struct sb_info *sb,  __u16 num){
	return (mrg_head.free_blk_cnt >= num);
}

/*
 * Check the block blk having space in it. the lucky thing is when an raw inode is
 * being inserted, the space checkings are all on the block blk.
 * The ctt space are required one slot and the ptr space are required 2*lvl slots
 * ==> spc > one ctt slot + 2*lvl ptr slots
 */
int blk_spc_chk(__u16 blk, __u8 lvl){
	return blk_ctt_spc_chk(blk, 1) && blk_ptr_spc_chk(blk, 2*lvl);
}

int blk_fit_spc_chk(__u16 blk, __u8 lvl){
	return blk_ctt_spc_chk(blk, BLK_RSRV_SPC) && blk_ptr_spc_chk(blk, 2*lvl);
}

/*
 * get from the free list for free block with retruning block number
 * the block number start from and returning FH_NULL, if no free block
 */
int free_blk_get(){
	__u32 blk;

	blk = __find_first_bit(flash_blk_bmp, TOTAL_BLK_NUM);
	return blk ? blk - 1 : FH_NULL;
}

/*
 * set the the blkth block to dead state, and the block number start from 0
 */
void dirty_blk_set(__u16 blk){
	if(blk >= TOTAL_BLK_NUM){
		printk(KERN_INFO "dead_block_set: too large block number!!!\n");
		return;
	}
	
	__set_bit_zero(flash_blk_bmp, blk + 1);
}

__u16 blk_next_get(__u16 blk){
	return ptr_follow(blk, BLK_NXT_IDX, NULL);
}

__u16 blk_prnt_get(__u16 blk){
	return ptr_follow(blk, BLK_PRNT_IDX, NULL);
}

void erase_blk(__u16 blk){	
	memset(flash_drive[blk], 0xFF, BLOCK_SIZE);
}

void rels_blk(__u16 blk){
	mrg_head.free_blk_cnt++;
	state_nr.free_blk_cnt++;
//	mrg_lst_rm(blk);
	erase_blk(blk);
	__set_bit_one(flash_blk_bmp, blk + 1);
}


/*
 * initial the block blk with nb for indexing next block and return anchored node index number
 * blk : block number
 * nb : index linking next block
 * return : anchored node index number
 */
void blk_init(__u16 blk, __u16 pb, __u16 nb){
	struct ptrptr ptrptr = {.next = FH_NULL};
	int i;

	ptrptr.cur = pb;
	ptrptr_write(blk, &ptrptr); /* idx = 0 for block list to link previous block */
	ptrptr.cur = nb;
	ptrptr_write(blk, &ptrptr);	/* idx = 1 for block list to link next block */
	ptrptr.cur = FH_NULL;
	for(i = 0; i < MAX_LEVEL; i++){
		ptrptr_write(blk, &ptrptr); /* idx = 2~8 for thread  */
	}
}

/*
int blk_lst_update(__u16 blk, __u16 nb){
	ptrptr_update(blk, BLK_LST_IDX, nb);
}
*/ 

void init_core_struct(){
	int i;
	
	memset(flash_blk_bmp, 0xFF, TOTAL_BLK_NUM >> 3);
	dirty_blk_set(TEMP_BLK_IDX);
	dirty_blk_set(HEAD_BLK_IDX);
	dirty_blk_set(M1_INIT_IDX);
	dirty_blk_set(M2_INIT_IDX);
	
	/* init lru list and it's head*/
	for(i = 0; i < TOTAL_BLK_NUM; i++){
		mrg_lst[i].stbl_cost = 0;
		mrg_lst[i].next = 0xFFFF;
		mrg_lst[i].prev = 0xFFFF;
		mrg_lst[i].blk_obj_num = 0;
		mrg_lst[i].type = BLK_TYPE_FREE;
	}

	mrg_lst[HEAD_BLK_IDX].blk_obj_num = 1;
	mrg_lst[HEAD_BLK_IDX].next = 0xFFFF;
	mrg_lst[HEAD_BLK_IDX].prev = 0xFFFF;
	mrg_lst[HEAD_BLK_IDX].stbl_cost = 0;
	mrg_lst[HEAD_BLK_IDX].type = BLK_TYPE_META;
	
	mrg_head.m_start = HEAD_BLK_IDX;
	mrg_head.m_end = HEAD_BLK_IDX;
	mrg_head.free_blk_cnt = TOTAL_BLK_NUM - BLK_PRSRV_NUM - 1;

	state_nr.free_blk_cnt = TOTAL_BLK_NUM - BLK_PRSRV_NUM - 1;
	state_nr.ctt_valid_cnt = 0;
	state_nr.ptr_valid_cnt = 0;
	state_nr.ctt_read_cnt = 0;
	state_nr.ctt_write_cnt = 0;
	state_nr.ptr_read_cnt = 0;
	state_nr.ptr_write_cnt = 0;
	state_nr.scrb_cnt = 0;
	state_nr.gc_cnt = 0;
	state_nr.mrg_cnt = 0;
	state_nr.ctt_splt_cnt = 0;
	state_nr.ptr_splt_cnt = 0;
}

