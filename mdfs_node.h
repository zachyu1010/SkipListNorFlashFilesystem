#ifndef _MDFS_NODE_H
#define _MDFS_NODE_H
#include "flash_drive.h"

#define P 0.2  //0x7fffffff
#define MAX_LEVEL 6
#define NAME_SIZE (25 - 8)

#define TESTFS_PREROOT_INO 0
#define TESTFS_ROOT_INO 1

#define HALF_SPLIT 0
#define PIVOT_SPLIT 1
#define RANDOM_SPLIT 2
#define MEG_CHK 0


#define __u32 unsigned int
#define __u16 unsigned short
#define __u8 unsigned char

#pragma pack(1)
struct ptrptr {
	__u16 cur;
	__u16 next;
};

#pragma pack(1)
struct mdfs_raw_inode {
	__u8 tag1[4];
	__u32 ino;        /* Inode number */
	__u32 pino;       /* Parent's inode number. part of key scheme */
	__u32 mode;       /* The file's type or mode */
	__u16 uid;        /* The file's owner.  */
	__u16 gid;        /* The file's group.  */
	__u32 atime;      /* Last access time.  */
	__u32 mtime;      /* Last modification time.  */
	__u32 ctime;      /* Creation time.  */
	__u32 dsize;      /* Size of the node's data.  */
	__u8 nsize;       /* Name length.  */
	__u8 nlink;       /* Number of links.  */
	__u8 level;		  /* height of level */
	__u16 forward[MAX_LEVEL]; /* pointer to array of pointers */
	__u8 name[NAME_SIZE];  /* name of the raw inode. part of key scheme */
	__u8 tag2[4];
};

struct sb_info {
	__u16 blk_idx;
	__u16 tmp_idx;	/* reserved for garbage collection */
	__u16 m1_idx;	/* reserved for merge action */
	__u16 m2_idx;	/* reserved for merge action */
	__u16 free_blk_cnt;
	__u8 level;
};

typedef struct state_nr {
	__u32 free_blk_cnt;
	__u32 ctt_valid_cnt;
	__u32 ptr_valid_cnt;
	__u32 ctt_read_cnt;
	__u32 ctt_write_cnt;
	__u32 ptr_read_cnt;
	__u32 ptr_write_cnt;
	__u32 scrb_cnt;
	__u32 gc_cnt;
	__u32 splt_cnt;
	__u32 mrg_cnt;
	__u32 ptr_splt_cnt;
	__u32 ctt_splt_cnt;
} state_nr_t;

typedef struct lru_elmt{
	__u16 stbl_cost;
	__u16 prev;
	__u16 next;
	__u16 blk_obj_num;
	__u8  type;
} lru_elmt_t;

typedef struct {
	__u16 m_start;	/* metadata */
	__u16 m_end;
	__u16 d_start; 
	__u16 d_end;	/* data */
	__u16 free_blk_cnt;
} mrg_head_t;



/* raw_inode operations */
//extern struct mdfs_raw_inode *raw_inode_alloc(int level);
extern int raw_inode_insert(struct sb_info *sb, struct mdfs_raw_inode *mri);
extern int raw_inode_delete(struct sb_info *sb, __u32 pino, __u8 *name);
extern int raw_inode_cmp(unsigned int p1, const __u8 *name1, __u32 p2, const __u8 *name2);
extern struct mdfs_raw_inode *raw_inode_alloc(void);
extern __u16 random_level(void);

/* block operations */
extern int garbage_collect(struct sb_info *sb, __u16 *cb, struct ptrptr *cut);
extern void ptr_scrub(struct sb_info *sb, __u16 cb);
int merge_action_1(struct sb_info *sb, __u16 pb, __u16 cb, __u16 nb);
int merge_action_2(struct sb_info *sb, __u16 pb, __u16 cb, __u16 nb);
/* flash operations */
extern struct sb_info *sb_init(void);
extern void show_node(struct sb_info *sb);
extern void state(struct sb_info *sb);

/* global variable */
extern struct state_nr state_nr;
extern lru_elmt_t mrg_lst[TOTAL_BLK_NUM];
extern mrg_head_t mrg_head;
extern __u32 age_wrt_num;

/* macro function */
#define is_ptr_export(ptr) ((ptr) == FH_XPRT)
//#define mark_ptr_export(ptr) (ptr | FH_XPRT)
#define clear_ptr_export(ptr) (ptr & 0x7FFF)
#endif 
