#ifndef _UTILITY_H_
#define _UTILITY_H_

/* utility operaitons */
/* block content bitmap operations */
extern unsigned int blk_ctt_bmp_ffs(__u32 blk);
extern void blk_ctt_bmp_set_zero(__u32 blk, __u32 idx);

/* block pointer bitmap operations */
extern __u32 blk_ptr_bmp_ffs(__u32 blk);
extern void blk_ptr_bmp_set_zero(__u32 blk, __u32 idx);

/* pointer operations */
extern __u16 ptrptr_cur_read(__u16 blk, __u16 idx, __u16 *last);
extern __u16 ptrptr_update(__u16 blk, __u16 last, __u16 cur);
extern __u16 ptrptr_write(__u32 blk, struct ptrptr *ptrptr);

/* inode operations */
extern int raw_inode_read(__u16 blk, struct mdfs_raw_inode *mri, __u16 idx);
extern int raw_inode_write(__u16 blk, struct mdfs_raw_inode *mri, __u16 idx);

/* block operations */
extern int blk_spc_chk(__u16 blk, __u8 lvl);
extern int blk_ptr_spc_chk(__u16 blk, __u16 num);
extern int blk_ctt_spc_chk(__u16 blk, __u16 num);
extern __u16 blk_next_get(__u16 blk);
extern int free_blk_get();
extern void dirty_blk_set(__u16 blk);
extern void erase_blk(__u16 blk);
extern void rels_blk(__u16 blk);
extern void blk_init(__u16 blk, __u16 pb, __u16 nb);

/* init function */
extern void init_core_struct();

/* miscellaneous operations */
extern void memdump(unsigned char *buf, int len);
extern int fsck(struct sb_info *sb);

/* macro function */
#define blk_nxt_update(blk, nb) ptrptr_update(blk, BLK_NXT_IDX, nb)
#define blk_prnt_update(blk, pb) ptrptr_update(blk, BLK_PRNT_IDX, pb);


#endif


