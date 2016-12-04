//#define ESS_MDFS
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <time.h>
//#include "standard.h"
//#include "rand.h"
#include "mdfs_node.h"
#include "macro.h"
#include "flash_drive.h"
#include "utility.h"

state_nr_t state_nr;
lru_elmt_t mrg_lst[TOTAL_BLK_NUM];
mrg_head_t mrg_head;
__u32 age_wrt_num;
/* macro function*/


/* miscellaneous operations */
void init_cut2thread(__u16 *cut){
	int i;
	
	for(i = 0; i < MAX_LEVEL; i++){
		cut[i] = BLK_ANCHOR_IDX + i;
	}
}
#if 0
void mrg_lst_tail_add(__u16 blk){
	__u16 end;

	mrg_lst[blk].next = FH_NULL;
	if(mrg_head.m_end != FH_NULL)
		mrg_lst[mrg_head.m_end].next = blk;
	mrg_lst[blk].prev = mrg_head.m_end;
	mrg_head.m_end = blk;
	
	if(mrg_head.m_start == FH_NULL)
		mrg_head.m_start = blk;

	if(mrg_head.m_end == FH_NULL)
		mrg_head.m_end = blk;
}
#endif

void count_ctt_splt(){
	state_nr.ctt_splt_cnt++;
}

void count_ptr_splt(){
	state_nr.ptr_splt_cnt++;
}

void count_splt(__u16 blk, __u16 ptr){
	if(!blk_ctt_spc_chk(blk, 1)){
		state_nr.ctt_splt_cnt++;
	}

	if(!blk_ptr_spc_chk(blk, 2*ptr)){
		state_nr.ptr_splt_cnt++;
	}
}


void mrg_lst_tail_add(__u16 blk){
	__u16 end;

	mrg_lst[blk].next = FH_NULL;
	if(mrg_head.m_end != FH_NULL)
		mrg_lst[mrg_head.m_end].next = blk;
	
	mrg_lst[blk].prev = mrg_head.m_end;
	mrg_head.m_end = blk;
	
	if(mrg_head.m_start == FH_NULL)
		mrg_head.m_start = blk;
}

void mrg_lst_add(__u16 blk){
	__u16 start;

	mrg_lst[blk].prev = FH_NULL;
	mrg_lst[blk].next = mrg_head.m_start;
	
	if(mrg_head.m_start != FH_NULL)
		mrg_lst[mrg_head.m_start].prev = blk;
	
	mrg_head.m_start = blk;

	if(mrg_head.m_end == FH_NULL)
		mrg_head.m_end = blk;
}

void mrg_lst_elmt_reset(__u16 blk){
	mrg_lst[blk].blk_obj_num = 0;
	mrg_lst[blk].next = FH_NULL;
	mrg_lst[blk].prev = FH_NULL;
	mrg_lst[blk].stbl_cost = 0;
	mrg_lst[blk].type = BLK_TYPE_FREE;
}

void mrg_lst_rm(__u16 blk){
	if(mrg_lst[blk].prev != FH_NULL)
		mrg_lst[mrg_lst[blk].prev].next = mrg_lst[blk].next;
	else
		mrg_head.m_start = mrg_lst[blk].next;

	if(mrg_lst[blk].next != FH_NULL)
		mrg_lst[mrg_lst[blk].next].prev = mrg_lst[blk].prev;
	else
		mrg_head.m_end = mrg_lst[blk].prev;

	mrg_lst_elmt_reset(blk);
}

void mrg_lst_age(){
	__u16 idx = mrg_head.m_start;
	
	age_wrt_num++;
	if(age_wrt_num >= AGE_WRI_NUM){
		while(idx != FH_NULL){
//			mrg_lst[idx].stbl_cost = (mrg_lst[idx].stbl_cost >= AGE_FACTOR)
//									? mrg_lst[idx].stbl_cost - AGE_FACTOR
//									: 0;
			mrg_lst[idx].stbl_cost = (mrg_lst[idx].stbl_cost) >> 1;
			idx = mrg_lst[idx].next;
		}
		age_wrt_num == 0;
	}
}

void mrg_lst_elmt_sbstt(__u16 dst, __u16 src){
	mrg_lst[dst].blk_obj_num = mrg_lst[src].blk_obj_num;
	mrg_lst[dst].next = mrg_lst[src].next;
	mrg_lst[dst].prev = mrg_lst[src].prev;
	mrg_lst[dst].stbl_cost = mrg_lst[src].stbl_cost;
	mrg_lst[dst].type = mrg_lst[src].type;

	if(mrg_lst[src].prev != FH_NULL)
		mrg_lst[mrg_lst[src].prev].next = dst;

	if(mrg_lst[src].next != FH_NULL)
		mrg_lst[mrg_lst[src].next].prev = dst;

	if(mrg_lst[src].prev == FH_NULL)
		mrg_head.m_start = dst;
}


void mrg_lst_print(__u16 idx){
	printk(KERN_INFO "\t\t==== mergible list ==== \t\t\n");
	while(1){
		if(idx == FH_NULL){
			printk(KERN_INFO "FH_NULL\n");
			break;
		}
		
		printk(KERN_INFO "(%d, %d, %d)->", idx, mrg_lst[idx].blk_obj_num, mrg_lst[idx].stbl_cost);
		idx = mrg_lst[idx].next;
	}
}



void blk_obj_num_inc(__u16 blk){
	mrg_lst[blk].blk_obj_num++;
}

void blk_obj_num_dec(__u16 blk){
	mrg_lst[blk].blk_obj_num--;
}

__u16 blk_obj_num_get(__u16 blk){
	return mrg_lst[blk].blk_obj_num;
}

void blk_obj_num_set(__u16 blk, __u16 value){
	mrg_lst[blk].blk_obj_num = value;
}

void blk_stbl_cost_inc(__u16 blk){
	//mrg_lst[blk].stbl_cost += AGE_FACTOR;
	mrg_lst[blk].stbl_cost = (mrg_lst[blk].stbl_cost << 1) + 1;
	if(mrg_lst[blk].stbl_cost > STABLE_SCALE)
		mrg_lst[blk].stbl_cost = STABLE_SCALE;
}

__u16 blk_stbl_cost_get(__u16 blk){
	return mrg_lst[blk].stbl_cost;
}

void blk_stbl_cost_set(__u16 blk, __u16 value){
	value = (value > STABLE_SCALE) ? STABLE_SCALE : value;
	mrg_lst[blk].stbl_cost = value;
}

int is_blk_stable(__u16 blk){
	__u16 chip = (BLK_INODE_NUM - blk_obj_num_get(blk)) / NONE_VALID_PER_CHIP;
	
	return (chip > blk_stbl_cost_get(blk)) ? 1 : 0;
//	return 0;
}

int find_mrg_blk_from(__u16 start, __u16 pb, __u16 cb, __u16 nb){
	__u16 tpb, tnb, tcb = start;
	__u16 scnt, chip, total;
	
	if(start == FH_NULL){
		tcb = mrg_head.m_end;
	}

	while(tcb != FH_NULL){
		tpb = blk_prnt_get(tcb);
		tnb = blk_next_get(tcb);
		if((tnb == pb) || (tcb == pb) || (tcb == cb) || (tcb == nb))
			goto next;

		if(tnb != FH_NULL){
			total = (mrg_lst[tcb].blk_obj_num + mrg_lst[tnb].blk_obj_num);
			if(total <= BLK_INODE_NUM){
#if MEG_CHK				
				scnt = mrg_lst[tcb].splt_cnt + mrg_lst[tnb].splt_cnt;
				chip = (BLK_INODE_NUM - total) / NONE_VALID_PER_CHIP;
				if((chip >= scnt)){
					return tcb;
				}
#else
				return tcb;
#endif
			}
		}
next:
		tcb = mrg_lst[tcb].prev;
	}

	return FH_NULL;
}



__u16 blk_ctt_alloc(__u16 blk){
	__u16 idx;
		
	idx = blk_ctt_bmp_ffs(blk);
	blk_ctt_bmp_set_zero(blk, idx);
	return idx;
}

__u16 blk_ptr_alloc(__u16 blk){
	__u16 idx;
		
	idx = blk_ptr_bmp_ffs(blk);
	blk_ptr_bmp_set_zero(blk, idx);
	return idx;
}

__u16 free_mblk_alloc(){
	__u16 idx;
	
	idx = free_blk_get();
	if(idx != FH_NULL){
		dirty_blk_set(idx);
		mrg_head.free_blk_cnt--;
		state_nr.free_blk_cnt--;
		
		mrg_lst[idx].type = BLK_TYPE_META;
	}
	return idx;
}

#if 0
void search_pre(struct sb_info *sb, __u16 *_pb, __u16 *_cb, __u16 *_nb,
					__u32 pino, __u8 *name, struct ptrptr *_cut){
	struct mdfs_raw_inode tmp, x;
	struct ptrptr cut[MAX_LEVEL];
	__u16 last, nb, pb, cb, tb, k, tk;
	int i, j, m;

	pb = FH_NULL;
	cb = sb->blk_idx;
	nb = blk_next_get(cb);
//	memset(cut, 0, MAX_LEVEL * sizeof(struct ptrptr));

	for(i = 0; i < MAX_LEVEL; i++){  /////+
		cut[i].cur = ptrptr_cur_read(cb, BLK_ANCHOR_IDX + i, NULL);
		cut[i].next = BLK_ANCHOR_IDX + i;
	}

	raw_inode_read(cb, &x, ptrptr_cur_read(cb, BLK_ANCHOR_IDX, NULL));
	/* find and get which block and position to insert the raw inode */
	for(i = MAX_LEVEL - 1; i >= 0; i--){
		tb = cb;
		k = tk = ptrptr_cur_read(tb, x.forward[i], &last);
		while(k != FH_NULL){
			if(is_ptr_export(k)){
				tb = nb;
				tk = ptrptr_cur_read(tb, BLK_ANCHOR_IDX + i , NULL);

				if(tk == FH_NULL) break;
			}
			raw_inode_read(tb, &tmp, tk);
			if(raw_inode_cmp(tmp.pino, tmp.name, pino, name) >= 0)
				break;

			x = tmp;
			if(tb == nb){
				pb = cb;
				cb = tb;
				nb = blk_next_get(cb);
				/* cross block, and reset cut */
				for(j = 0; j < MAX_LEVEL; j++){
					cut[j].cur = ptrptr_cur_read(cb, BLK_ANCHOR_IDX + j, NULL);
					cut[j].next = BLK_ANCHOR_IDX + j;
				}
			}
			tk = k = ptrptr_cur_read(tb, x.forward[i], &last);
		}
		cut[i].next = last;
		cut[i].cur = k;
	}
	
	if(_pb) *_pb = pb;
	if(_cb) *_cb = cb;
	if(_nb) *_nb = nb;
	
	if(_cut) 
		memcpy(_cut, cut, sizeof(struct ptrptr)*MAX_LEVEL);
}

#endif

void search_pre(struct sb_info *sb, __u16 *_pb, __u16 *_cb, __u16 *_nb,
					__u32 pino, __u8 *name, struct ptrptr *_cut){
	struct mdfs_raw_inode tmp, x;
	struct ptrptr cut[MAX_LEVEL];
	__u16 last, nb, pb, cb, tb, k, tk;
	int i, j, m;

	pb = FH_NULL;
	cb = sb->blk_idx;
	nb = blk_next_get(cb);
//	memset(cut, 0, MAX_LEVEL * sizeof(struct ptrptr));

	for(i = 0; i < MAX_LEVEL; i++){
		cut[i].cur = ptrptr_cur_read(cb, BLK_ANCHOR_IDX + i, NULL);
		cut[i].next = BLK_ANCHOR_IDX + i;
	}

	raw_inode_read(cb, &x, ptrptr_cur_read(cb, BLK_ANCHOR_IDX, NULL));
	/* find and get which block and position to insert the raw inode */
	for(i = MAX_LEVEL - 1; i >= 0; i--){
		tb = cb;
		k = tk = ptrptr_cur_read(tb, x.forward[i], &last);
		while(k != FH_NULL){
			if(is_ptr_export(k)){
				tb = nb;
thr_blk:
				tk = ptrptr_cur_read(tb, BLK_ANCHOR_IDX + i , NULL);
				/* the pointer at the level of the thread is exporting, 
							so through the block tb and go to next block */
				if(is_ptr_export(tk)){	
					tb = blk_next_get(tb);
					goto thr_blk;
				}
				
				if(tk == FH_NULL) break;
			}
			raw_inode_read(tb, &tmp, tk);
			if(raw_inode_cmp(tmp.pino, tmp.name, pino, name) >= 0)
				break;

			x = tmp;
			if(cb != tb){
				pb = blk_prnt_get(tb);
				nb = blk_next_get(tb);
				cb = tb;
				/* cross block, and reset cut */
				for(j = 0; j < x.level; j++){
					cut[j].cur = ptrptr_cur_read(tb, x.forward[i], &last);
					cut[j].next = last;
				}
				
				for(j = x.level; j < MAX_LEVEL; j++){
					cut[j].cur = ptrptr_cur_read(tb, BLK_ANCHOR_IDX + j, NULL);
					cut[j].next = BLK_ANCHOR_IDX + j;
				}

			}
			tk = k = ptrptr_cur_read(tb, x.forward[i], &last);
		}
		cut[i].next = last;
		cut[i].cur = k;
	}
	
	if(_pb) *_pb = pb;
	if(_cb) *_cb = cb;
	if(_nb) *_nb = nb;
	
	if(_cut) 
		memcpy(_cut, cut, sizeof(struct ptrptr)*MAX_LEVEL);
}


__u16 do_move(struct sb_info *sb, __u16 cb, __u16 sp, 
						__u16 total, __u16 pivot, __u16 term, __u16 *old, __u16 *new){
	struct mdfs_raw_inode x;
	__u16 k, idx, next, cut[MAX_LEVEL] ,count = 0;
	int i;

	if(new == NULL)
		init_cut2thread(cut);
	else
		memcpy(cut, new, sizeof(__u16)*MAX_LEVEL);

	next = old[0];
	while(1) {	
		k = ptrptr_cur_read(cb, next, NULL);
		if((k == FH_NULL) || (k ==FH_XPRT)) // end of the block
			break;

		if ((count == total) || (k == pivot)) // meet the conditions
			break;

		raw_inode_read(cb, &x, k);
		next = x.forward[0];
		idx = blk_ctt_alloc(sp);
		for(i = 0; i < x.level; i++){
			ptrptr_update(sp, cut[i], idx);
		}

		for(i = 0; i < x.level; i++){
			old[i] = x.forward[i];
			x.forward[i] = blk_ptr_alloc(sp);
			cut[i] = x.forward[i];
		}
		count++;
		raw_inode_write(sp, &x, idx);
	}

	for(i = 0; i < MAX_LEVEL; i++){
		ptrptr_update(sp, cut[i], term);
	}

	if(new != NULL){
		memcpy(new, cut, sizeof(__u16)*MAX_LEVEL);
	}

	mrg_lst[sp].blk_obj_num += count;
	return count;
}

/* raw inode operations */
int raw_inode_cmp(__u32 p1, const __u8 *name1, __u32 p2, const __u8 *name2) {
	return  (p1 > p2) ? 1 : 
			(p1 < p2) ? -1 : 
			strcmp((char *)name1, (char *)name2);
}

#if 0
__u16 random_level(void) {
	__u16 lvl = 1;

	while(rand32() < P && lvl < MAX_LEVEL)
		lvl++;
	return lvl;
} 
#else

float frand(){
   return (float) rand()/RAND_MAX;
//	return 0;
}

__u16 random_level() {
    static int first = 1;
    int lvl = 1;

    if(first) {
        srand( (unsigned)time( NULL ) );

        first = 0;
    }

    while(frand() < P && lvl < MAX_LEVEL)
		lvl++;

    return lvl;
} 
#endif


__u16 random_split(__u16 num){
	__u16 count;
	if(num == 2)
		return 1;
	
//	count = rand32() % num;
	count = rand() % num;
	if(count == 0)
		return 1;
	
	return  count;
}

struct mdfs_raw_inode *raw_inode_alloc()
{
	struct mdfs_raw_inode* ri = (struct mdfs_raw_inode *)kmalloc(sizeof(struct mdfs_raw_inode), GFP_KERNEL);
	
	if(!ri)
		goto failed; 

	memset(ri, 0, sizeof(struct mdfs_raw_inode));
	memset(ri->tag1, 0xAA, 4);
	memset(ri->tag2, 0xBB, 4);
	memset(ri->forward, 0xFF, sizeof(ri->forward));
	return ri;
failed:
	return 0;
}

/*
 * return head of child list, if any            
 * return NULL, otherwise                       
 */

/*
struct mdfs_raw_inode *child_list(struct sb_info *sb, __u32 ino){
	struct mdfs_raw_inode *x = sb->head;
	int i;
	
	for(i = sb->head->level - 1; i >= 0; i--) {
		while(x->forward[i] != NULL && x->forward[i]->pino < ino) {
			x = x->forward[i];
		}
	}
	x = x->forward[0];

	if(x != NULL && ino == x->pino){
		return x;
	}

	return NULL;
}
*/

/*
 * check if the raw inode have child,
 * return 0, if having children,
 * return 1, if having no children,
 */

/*
int child_empty(struct sb_info *sb, __u32 ino){
	return (child_list(sb, ino) == NULL) ? 1 : 0;
}
*/

/*
void child_print(struct sb_info *sb, __u32 ino) {
	struct mdfs_raw_inode *x = sb->head;

	x = child_list(sb, ino);

	printk(KERN_INFO "{");
	while(x != NULL && ino == x->pino){
		printk(KERN_INFO "%d:%d:%s", x->pino, x->ino, x->name);
		x = x->forward[0];
		
		if(x!= NULL && ino == x->pino)
			printk(KERN_INFO ", ");
	}
	printk(KERN_INFO "}\n");
}

*/

int raw_inode_search(struct sb_info *sb, __u16 *_pb, __u16 *_cb, __u16 *_nb, __u32 pino, __u8 *name,
						struct mdfs_raw_inode *mri, struct ptrptr *cut1, struct ptrptr *cut2){
	struct mdfs_raw_inode tmp;
	struct ptrptr cut[MAX_LEVEL];
	__u16 last, nb, pb, cb, k;
	int i, err = -ENOENT;

	search_pre(sb, &pb, &cb, &nb, pino, name, cut);	

	
	k = cut[0].cur;
	if(k == FH_NULL)
		goto out;

	if(is_ptr_export(k)){
		int tk;
		
		tk = ptrptr_cur_read(nb, BLK_ANCHOR_IDX, NULL);
		raw_inode_read(nb, &tmp, tk);
	}
	else
		raw_inode_read(cb, &tmp, k);

	if(raw_inode_cmp(tmp.pino, tmp.name, pino, name) != 0)
		goto out;

	if(is_ptr_export(k)){
		if(cut2 != NULL)
			memcpy(cut2, cut, sizeof(struct ptrptr)*MAX_LEVEL);
		
		for(i = 0; i < MAX_LEVEL; i++){
			cut[i].cur = ptrptr_cur_read(nb, BLK_ANCHOR_IDX + i, &last);
			cut[i].next = last;
		}

		pb = cb;
		cb = nb;
		nb = blk_next_get(cb);
	}

	if(mri != NULL) *mri = tmp;
	if(_pb != NULL) *_pb = pb;
	if(_cb != NULL) *_cb = cb;
	if(_nb != NULL) *_nb = nb;
	if(cut1 != NULL)
		memcpy(cut1, cut, sizeof(struct ptrptr)*MAX_LEVEL);

	err = 0;
out: 
	return err;
}

/*
 * insert a node into raw inode list
 * if success, return 0;
 * if fail, return error-value
 */
int raw_inode_insert(struct sb_info *sb, struct mdfs_raw_inode *mri) {
	struct mdfs_raw_inode tmp, x;
	struct ptrptr cut[MAX_LEVEL];	/* .cur record the index of next raw inode */
									/* .next record the index of pointer pointing to next raw inode */										
	__u16 last, nb, pb, cb, k;
	int i, lvl, err = -EEXIST;

	if(mri->nsize == 0) 
		return -EINVAL;
	/*
	 * cb is the block corresponding to current raw inode 
	 * pb is the previous block corresponding to block cb
	 * nb is the next block corresponding to block cb 
	*/
	
	search_pre(sb, &pb, &cb, &nb, mri->pino, mri->name, cut);
	k = cut[0].cur;
	if(k != FH_NULL){
		if(is_ptr_export(k)){
			int tk;
			
			tk = ptrptr_cur_read(nb, BLK_ANCHOR_IDX, NULL);
			raw_inode_read(nb, &tmp, tk);	
		}
		else
			raw_inode_read(cb, &tmp, k);
	}

	if((k == FH_NULL) || (raw_inode_cmp(tmp.pino, tmp.name, mri->pino, mri->name) != 0)) {
		struct ptrptr ptrptr = {.next = 0xFFFF};
		__u16 idx;

		mri->level = random_level();

		if((k == FH_NULL) || (k == FH_XPRT)){	/* sequential insert */
			if(!blk_fit_spc_chk(cb, mri->level)){
				int sp;
				err = -ENOSPC;
				
				if(!free_blk_chk(sb, 1)){
					printk(KERN_INFO "raw_inode_insert: 1. flash has no space...\n");
					printk(KERN_INFO "do merge_action_1...\n");
					err = merge_action_1(sb, pb, cb, nb);
					if(err < 0){
						printk(KERN_INFO "raw_inode_insert: 1. merge_action_1 failed...\n");
						goto out;
					}
				}
				sp = free_mblk_alloc();
				/* check if current block has free pointer to link next block ib */
				if(!blk_ptr_spc_chk(cb, 1)){
					printk(KERN_INFO "raw_inode_insert: 1. current block %d has no pointer for block list..\n", cb);
					printk(KERN_INFO "do pointer scrub...\n");
					ptr_scrub(sb, cb);
				}
				blk_init(sp, cb, nb);
				blk_nxt_update(cb, sp);
				if(nb != FH_NULL){					
					if(!blk_ptr_spc_chk(nb, 1)){
						printk(KERN_INFO "raw_inode_insert: 1. next block %d has no pointer for block list..\n", nb);
						printk(KERN_INFO "do pointer scrub...\n");
						ptr_scrub(sb, nb);
					}				
					blk_prnt_update(nb, sp);
				}

				idx = blk_ctt_alloc(sp);
				for(i = 0 ; i < MAX_LEVEL; i++){
					ptrptr_update(cb, cut[i].next, FH_XPRT);
				}

				ptrptr.cur = (nb == FH_NULL) ? FH_NULL : FH_XPRT;
				for(i = 0; i < mri->level; i++){
					ptrptr_update(sp, BLK_ANCHOR_IDX + i, idx);
					mri->forward[i] = ptrptr_write(sp, &ptrptr);
				}

				for(i = mri->level; i < MAX_LEVEL; i++){
					ptrptr_update(sp, BLK_ANCHOR_IDX + i,  ptrptr.cur);
				}
				raw_inode_write(sp, mri, idx);
				blk_obj_num_inc(sp);  //the block's valid data plus one
				mrg_lst_add(sp);
				err = 0;
				state_nr.ctt_valid_cnt++;
				state_nr.ptr_valid_cnt += mri->level;
				goto out;
			}
			else{
				goto general;
			}
		}
		else if(!blk_spc_chk(cb, mri->level)){
			if(!is_blk_stable(cb)){  /* if not stable, do split  */
				__u16 sp1, sp2, term = (nb != FH_NULL) ? FH_XPRT : FH_NULL,
					old[MAX_LEVEL], new[MAX_LEVEL]; /* temp idx of the current node to link the fllowing node */
				err = -ENOSPC;
				
				if(!free_blk_chk(sb, 2)){	/* no two free block */
					printk(KERN_INFO "raw_inode_insert: 2. no enough 2 space on flash for split...\n");
					printk(KERN_INFO "raw_inode_insert: do merge_action_2...\n");
					err = merge_action_2(sb, pb, cb, nb);
					if(err < 0){
						printk(KERN_INFO "raw_inode_insert: 2. merge_action_2 faild...\n");
						goto out;
					}
				}	
				/* parent block need one pointer to link sp1 */

				if((pb != FH_NULL) && !blk_ptr_spc_chk(pb, 1)){
					printk(KERN_INFO "raw_inode_insert: 2. parent block %d has no pointer for block list..\n", pb);
					printk(KERN_INFO "do pointer scrub...\n");
					ptr_scrub(sb, pb);
				}

				sp1 = free_mblk_alloc();
				sp2 = free_mblk_alloc();
				if(pb == FH_NULL)
					sb->blk_idx = sp1;
				else
					blk_nxt_update(pb, sp1);	/* link block pb to block sp1 */
				
				blk_init(sp1, pb, sp2);			/* format block sp1 and link to block sp2 */
				blk_init(sp2, sp1, nb);			/* format block sp2 and link to block nb */
				if(nb != FH_NULL){
					if(!blk_ptr_spc_chk(nb, 1)){
						printk(KERN_INFO "raw_inode_insert: 2. next block %d has no pointer for block list..\n", nb);
						printk(KERN_INFO "do pointer scrub...\n");
						ptr_scrub(sb, nb);
					}				
					blk_prnt_update(nb, sp2);
				}

				//init start threads for sp1
				init_cut2thread(old);
				init_cut2thread(new);
				
				/* copy the data of the block cb before pivot to sp1 */
				do_move(sb, cb, sp1, FH_NULL, k, FH_NULL, old, new);
				idx = blk_ctt_alloc(sp1);
				
				for(i = 0; i < mri->level; i++){
					ptrptr_update(sp1, new[i], idx);
				}
				ptrptr.cur = 0xFFFF;
				for(i = 0; i < mri->level; i++){
					mri->forward[i] = ptrptr_write(sp1, &ptrptr);
					new[i] = mri->forward[i];
				}
				raw_inode_write(sp1, mri,idx);
				blk_obj_num_inc(sp1);
				for(i = 0; i < MAX_LEVEL; i++){
					ptrptr_update(sp1, new[i], FH_XPRT);
				}

				init_cut2thread(new);
				/* copy the remain data of the block cb including pivot to sp2 */
				do_move(sb, cb, sp2, FH_NULL, FH_NULL, term, old, new);
				blk_stbl_cost_set(sp1, blk_stbl_cost_get(cb));
				blk_stbl_cost_set(sp2, blk_stbl_cost_get(cb));
				
				count_splt(cb, mri->level);
					
				mrg_lst_add(sp1);
				mrg_lst_add(sp2);
				mrg_lst_rm(cb);
				rels_blk(cb); 
				err = 0;
				state_nr.ctt_valid_cnt++;
				state_nr.splt_cnt++;
				state_nr.ptr_valid_cnt += mri->level;
				goto out;
			}
			else{ /* if stable, do garbage collection */

				count_splt(cb, mri->level);
				
				err = garbage_collect(sb, &cb, cut);
				if(err < 0){
					printk(KERN_INFO "raw_inode_update: 3. garbage collect failed..\n");
					goto out;
				}

				goto after_gc;
			}
		}

general:
after_gc:
		idx = blk_ctt_alloc(cb);
		for(i = 0; i < mri->level; i++) {
			/* add new idxth of pointer list to the new raw inode */
			ptrptr.cur = cut[i].cur;
			/* mri->forward[i] is set to the block cut[i].cur to linking next block */
			mri->forward[i] = ptrptr_write(cb, &ptrptr);
			/* udate cut[i] to mri to linking mri */
			ptrptr_update(cb, cut[i].next, idx);
		}

		raw_inode_write(cb, mri, idx);
		blk_obj_num_inc(cb);  //the block's valid data plus one
		err = 0;
		state_nr.ctt_valid_cnt++;
		state_nr.ptr_valid_cnt += mri->level;
		goto out;
	}
	printk(KERN_INFO "raw_inode_insert: %s file or directory exist!\n", mri->name);
	err = -EEXIST;
out:
	return err;
}

int raw_inode_update(struct sb_info *sb, struct mdfs_raw_inode *mri) {
	struct mdfs_raw_inode x;
	struct ptrptr ptrptr = {.next = FH_NULL}, 
				cut[MAX_LEVEL];	/* .cur record the index of next raw inode 
								   .next record the index of pointer pointing to next raw inode */										
	__u16 last, nb, pb, cb, idx, start;
	int i, err = -ENOENT;

	if(mri->nsize == 0) 
		return -EINVAL;

	if((err = raw_inode_search(sb, &pb, &cb, &nb, mri->pino, mri->name, &x, cut, NULL)) < 0){
		printk(KERN_INFO "raw_inode_update: no such instance %s...\n", mri->name);
		goto out;
	}

	if(x.level != mri->level){
		mri->level = x.level;
//		printk(KERN_INFO "raw_inode_update: level conflict, force matching to level of older one\n");
	}

	if(!blk_spc_chk(cb, mri->level)){
		__u16 tt;
	
		if(!is_blk_stable(cb)){		/* if not stable, split the block */
			__u16 sp1, sp2, bl, k, half, count, rs, 
				old[MAX_LEVEL], new[MAX_LEVEL], term;

			if(blk_obj_num_get(cb) == 1){
				printk(KERN_INFO "raw_inonde_update: one valid metat data, direct garbage collection\n");

				count_splt(cb, mri->level);
				
				if((err = garbage_collect(sb, &cb, cut)) < 0){
					printk(KERN_INFO "raw_inode_update: garbage_collect failed...\n");
					goto out;
				}
				blk_stbl_cost_inc(cb);
				raw_inode_read(cb, &x, cut[0].cur);
				goto after_gc;
			}
			
			if(!free_blk_chk(sb, 2)){
				printk(KERN_INFO "raw_inode_update: no enough 2 space on flash for split...\n");
				printk(KERN_INFO "do merge_action_2...\n");

				err = merge_action_2(sb, pb, cb, nb);
				if(err < 0){			
					printk(KERN_INFO "raw_inode_update: merge_action_2 failed...\n");
					goto out;
				}
			}
			/* parent block need one pointer to link sp1 */
			if((pb != FH_NULL) && !blk_ptr_spc_chk(pb, 1)){
				printk(KERN_INFO "raw_inode_update: parent block %d has no pointer for block list..\n", pb);
				printk(KERN_INFO "do pointer scrub...\n");
				ptr_scrub(sb, pb);
			}
			/* current block need two free blocks */
			sp1 = free_mblk_alloc();
			sp2 = free_mblk_alloc();
			if(pb == FH_NULL)
				sb->blk_idx = sp1;
			else
				blk_nxt_update(pb, sp1);	/* link block pb to block sp1 */
			
			blk_init(sp1, pb, sp2);			/* format block sp1 and link to block sp2 */
			blk_init(sp2, sp1, nb);			/* format block sp2 and link to block nb */
			
			if(nb != FH_NULL){
				if(!blk_ptr_spc_chk(nb, 1)){
					printk(KERN_INFO "raw_inode_update: next block %d has no pointer for block list..\n", nb);
					printk(KERN_INFO "do pointer scrub...\n");
					ptr_scrub(sb, nb);
				}				

				blk_prnt_update(nb, sp2);
			}
			init_cut2thread(old);
			init_cut2thread(new);
		
			bl = blk_obj_num_get(cb);
			half = bl/2;
			k = cut[0].cur;

#define SPLIT_POLICY RANDOM_SPLIT
#if SPLIT_POLICY == PIVOT_SPLIT
			/* copy the data of the block cb before pivot to sp1 */

			count = do_move(sb, cb, sp1, FH_NULL, k, FH_NULL, old, new);
			term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
			ptrptr.cur = 0xFFFF;
			if(count < half){
				idx = blk_ctt_alloc(sp1);
//				ptrptr.cur = 0xFFFF;
				for(i = 0; i < mri->level; i++){
					ptrptr_update(sp1, new[i], idx);
					mri->forward[i] = ptrptr_write(sp1, &ptrptr);
					new[i] = mri->forward[i];
					ptr_follow(cb, x.forward[i], &old[i]);
				}
				
				raw_inode_write(sp1, mri, idx);
				blk_obj_num_inc(sp1);
				for(i = 0; i < MAX_LEVEL; i++){
					ptrptr_update(sp1, new[i], FH_XPRT);
				}

				init_cut2thread(new);
				/* copy the remain data of the block cb including pivot to sp2 */
				do_move(sb, cb, sp2, FH_NULL, FH_NULL, term, old, new);
			}
			else{
				for(i = 0; i < MAX_LEVEL; i++){
					ptrptr_update(sp1, new[i], FH_XPRT);
				}
				
				idx = blk_ctt_alloc(sp2);
				init_cut2thread(new);
				for(i = 0; i < mri->level; i++){
					ptrptr_update(sp2, new[i], idx);
				}
				
//				ptrptr.cur = 0xFFFF;
				for(i = 0; i < mri->level; i++){
					ptrptr_update(sp2, new[i], idx);
					mri->forward[i] = ptrptr_write(sp2, &ptrptr);
					new[i] = mri->forward[i];
					ptr_follow(cb, x.forward[i], &old[i]);
				}
				
				raw_inode_write(sp2, mri, idx);
				blk_obj_num_inc(sp2);

				do_move(sb, cb, sp2, FH_NULL, FH_NULL, term, old, new);				
			}			
#elif SPLIT_POLICY == RANDOM_SPLIT
			rs = random_split(bl);
			count = do_move(sb, cb, sp1, rs, k, FH_NULL, old, new);
			ptrptr.cur = FH_NULL;
			if(count < rs){
				idx = blk_ctt_alloc(sp1);
				for(i = 0; i < mri->level; i++){
					mri->forward[i] = ptrptr_write(sp1, &ptrptr);
					ptrptr_update(sp1, new[i], idx);
					ptr_follow(cb, x.forward[i], &old[i]);
					new[i] = mri->forward[i];
				}
				raw_inode_write(sp1, mri, idx);
				mrg_lst_age();
				blk_obj_num_inc(sp1);

				k = FH_NULL;
				do_move(sb, cb, sp1, (rs - count - 1), FH_NULL, FH_XPRT, old, new);
			}
			else{
				for(i = 0; i < MAX_LEVEL; i++){
					ptrptr_update(sp1, new[i], FH_XPRT);
				}
			}
			init_cut2thread(new);
			term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
			if(k == FH_NULL){	// find the updated object in pass1, so close the end with the variable term
				count = do_move(sb, cb, sp2, bl - rs, FH_NULL, term, old, new);
			}
			else{	// not fint the updated object in pass1, be sure it is in pass2
				count = do_move(sb, cb, sp2, bl - rs, k, FH_NULL, old, new);
				idx = blk_ctt_alloc(sp2);
				for(i = 0; i < mri->level; i++){
					//ptrptr.cur = ptrptr_cur_read(cb, x.forward[i], &last);
					//ptrptr.cur = FH_NULL;
					mri->forward[i] = ptrptr_write(sp2, &ptrptr);
					ptrptr_update(sp2, new[i], idx);
					ptr_follow(cb, x.forward[i], &old[i]);
					new[i] = mri->forward[i];
				}
				raw_inode_write(sp2, mri, idx);
				mrg_lst_age();
				blk_obj_num_inc(sp2);
				do_move(sb, cb, sp2, (bl - rs - count - 1), FH_NULL, term, old, new);
			}
			
#else

			count = do_move(sb, cb, sp1, half, k, FH_NULL, old, new);
			ptrptr.cur = FH_NULL;
			if(count < half){
				idx = blk_ctt_alloc(sp1);
				for(i = 0; i < mri->level; i++){
//					ptrptr.cur = FH_NULL;
					mri->forward[i] = ptrptr_write(sp1, &ptrptr);
					ptrptr_update(sp1, new[i], idx);
					ptr_follow(cb, x.forward[i], &old[i]);
					new[i] = mri->forward[i];
				}
				raw_inode_write(sp1, mri, idx);
				mrg_lst_age();
				blk_obj_num_inc(sp1);

				k = FH_NULL;
				do_move(sb, cb, sp1, (half - count - 1), FH_NULL, FH_XPRT, old, new);
			}
			else{
				for(i = 0; i < MAX_LEVEL; i++){
					ptrptr_update(sp1, new[i], FH_XPRT);
				}
			}
			init_cut2thread(new);
			term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
			if(k == FH_NULL){	// find the updated object in pass1, so close the end with the variable term
				count = do_move(sb, cb, sp2, bl - half, FH_NULL, term, old, new);
			}
			else{	// not fint the updated object in pass1, be sure it is in pass2
				count = do_move(sb, cb, sp2, bl - half, k, FH_NULL, old, new);
				idx = blk_ctt_alloc(sp2);
				for(i = 0; i < mri->level; i++){
					//ptrptr.cur = ptrptr_cur_read(cb, x.forward[i], &last);
					//ptrptr.cur = FH_NULL;
					mri->forward[i] = ptrptr_write(sp2, &ptrptr);
					ptrptr_update(sp2, new[i], idx);
					ptr_follow(cb, x.forward[i], &old[i]);
					new[i] = mri->forward[i];
				}
				raw_inode_write(sp2, mri, idx);
				mrg_lst_age();
				blk_obj_num_inc(sp2);
				do_move(sb, cb, sp2, (bl - half - count - 1), FH_NULL, term, old, new);
			}
#endif
			blk_stbl_cost_set(sp1, blk_stbl_cost_get(cb)*2);
			blk_stbl_cost_set(sp2, blk_stbl_cost_get(cb)*2);
			
			count_splt(cb, mri->level);
			
			state_nr.splt_cnt++;
			mrg_lst_add(sp1);
			mrg_lst_add(sp2);
			mrg_lst_rm(cb);
			rels_blk(cb);
			err = 0;
			goto out;
		}
		else{
			printk(KERN_INFO "raw_inode_update:2. no space, but stable, do garbage collection\n");

			count_splt(cb, mri->level);
			
			err = garbage_collect(sb, &cb, cut);
			if(err < 0){
				printk(KERN_INFO "raw_inode_update: gabage_collec failed...\n");
				goto out;
			}

			raw_inode_read(cb, &x, cut[0].cur);
			goto after_gc;
		}
	}
	
after_gc:
	
	idx = blk_ctt_alloc(cb);
	for(i = 0; i < mri->level; i++){
		ptrptr_update(cb, cut[i].next, idx);
		ptrptr.cur = ptrptr_cur_read(cb, x.forward[i], NULL);
		mri->forward[i] = ptrptr_write(cb, &ptrptr);
	}
	raw_inode_write(cb, mri, idx);
	mrg_lst_age();
	err = 0;
out:
	return err;
}

int raw_inode_delete(struct sb_info *sb, __u32 pino, __u8 *name) {
	struct mdfs_raw_inode x;
	struct ptrptr ptrptr = {.next = FH_NULL}, 
				cut[MAX_LEVEL], pre_cut[MAX_LEVEL];	/* .cur record the index of next raw inode 
								   .next record the index of pointer pointing to next raw inode */										
	__u16 nb, pb, cb, idx, start;
	int i, err = -ENOENT;

	if((err = raw_inode_search(sb, &pb, &cb, &nb, pino, name, &x, cut, pre_cut)) < 0){
		goto out;
	}

	/* if the deleted data in the block cb is only one. remove the block */
	if(blk_obj_num_get(cb) == 1){
		if(nb != FH_NULL){
			if(!blk_ptr_spc_chk(nb, 1)){
				printk(KERN_INFO "raw_inode_delete: next block %d has no pointer for block list..\n", nb);
				printk(KERN_INFO "do pointer scrub\n");
				ptr_scrub(sb, nb);
			}
			blk_prnt_update(nb, pb);
		}

		if(pb != FH_NULL){
			if(nb != FH_NULL){	/* next block exist, link parent block pb link to next block nb */
				if(!blk_ptr_spc_chk(pb, 1)){
					printk(KERN_INFO "raw_inode_delete: parent block %d has no pointer for block list..\n", pb);
					printk(KERN_INFO "do pointer scrub\n");
					ptr_scrub(sb, pb);
				}
			}
			else{	/* next block doesn't exist, set parent block pb end with FH_NULL */
				if(!blk_ptr_spc_chk(pb, MAX_LEVEL  + 1)){ /* one is for next block pointer */
					printk(KERN_INFO "raw_inode_delete: parent block %d has no pointer for block list..\n", pb);
					printk(KERN_INFO "do pointer scrub\n");
					ptr_scrub(sb, pb);
				}	
				for(i = 0; i < MAX_LEVEL;i++){
					ptrptr_update(pb, pre_cut[i].next, FH_NULL);
				}
			}
			blk_nxt_update(pb, nb);
		}
		else{  /* block cb is the head block, after remove block cb, the head block is next block nb */
			sb->blk_idx = nb;
		}
		
		mrg_lst_rm(cb);
		rels_blk(cb);
		
		err = 0;
		state_nr.ctt_valid_cnt--;
		state_nr.ptr_valid_cnt -= x.level;
		goto out;
	}

	if(!blk_ptr_spc_chk(cb, x.level)){
		if(!is_blk_stable(cb)){	/* if not stable, split the block */
			__u16 sp1, sp2, bl, k, half, count, 
				old[MAX_LEVEL], new[MAX_LEVEL], term;

			/* current block need two free blocks */
			if(!free_blk_chk(sb, 2)){
				printk(KERN_INFO "raw_inode_delete: no enough 2 space on flash for split...\n");
				printk(KERN_INFO "raw_inode_delete: do merge_action_2...\n");
				err = merge_action_2(sb, pb, cb, nb);
				if(err < 0){
					printk(KERN_INFO "raw_inode_delete: merge_action_2 failed...\n");
					goto out;
				}
			}
			/* parent block need one pointer to link sp1 */
			if((pb != FH_NULL) && !blk_ptr_spc_chk(pb, 1)){
				printk(KERN_INFO "raw_inode_delete: parent block %d has no pointer for block list..\n", pb);
				printk(KERN_INFO "do pointer_scrub...\n");
				ptr_scrub(sb, pb);
			}

			sp1 = free_mblk_alloc();
			sp2 = free_mblk_alloc();

			if(pb == FH_NULL)
				sb->blk_idx = sp1;
			else
				blk_nxt_update(pb, sp1);	/* link block pb to block sp1 */

			blk_init(sp1, pb, sp2);			/* format block sp1 and link to block sp2 */
			blk_init(sp2, sp1, nb);			/* format block sp2 and link to block nb */
			if(nb != FH_NULL){
				if(!blk_ptr_spc_chk(nb, 1)){
					printk(KERN_INFO "raw_inode_delete: next block %d has no pointer for block list..\n", nb);
					printk(KERN_INFO "do pointer scrub...\n");
					ptr_scrub(sb, nb);
				}	
				
				blk_prnt_update(nb, sp2);
			}

			init_cut2thread(new);
			init_cut2thread(old);
			
			bl = blk_obj_num_get(cb);
			half = bl/2;
			k = cut[0].cur;
#if PIVOT_SPLIT
			/* copy the data of the block cb before pivot to sp1 */
			count = do_move(sb, cb, sp1, FH_NULL, k, FH_XPRT, old, new);
			term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
			ptrptr.cur = 0xFFFF;
			for(i = 0; i < x.level; i++){
				ptr_follow(cb, x.forward[i], &old[i]);
			}

			init_cut2thread(new);
			/* copy the remain data of the block cb including pivot to sp2 */
			do_move(sb, cb, sp2, FH_NULL, FH_NULL, term, old, new);
#else
			count = do_move(sb, cb, sp1, half, k, FH_NULL, old, new);
			if(count < half){
				__u16 m;
				
				for(i = 0; i < x.level; i++){
//					ptrptr_cur_read(cb, x.forward[i], &last);
//					old[i] = last;
					ptrptr_cur_read(cb, x.forward[i], &old[i]);
				}

				k = FH_NULL;
				do_move(sb, cb, sp1, (half - count), FH_NULL, FH_XPRT, old, new);
			}
			else{
				for(i = 0; i < MAX_LEVEL; i++){
					ptrptr_update(sp1, new[i], FH_XPRT);
				}
			}
			
			init_cut2thread(new);
			term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
			if(k == FH_NULL){	// find the deleted object in pass1, so close the end with the variable term
				count = do_move(sb, cb, sp2, bl - half, FH_NULL, term, old, new);
			}
			else{	// not find the deleted object in pass1, be sure it is in pass2
				__u16 m;
				
				count = do_move(sb, cb, sp2, bl - half, k, FH_NULL, old, new);
				for(i = 0; i < x.level; i++){
					ptrptr_cur_read(cb, x.forward[i], &old[i]);
				}
				do_move(sb, cb, sp2, (bl - half - count), FH_NULL, term, old, new);
			}

#endif
			
			blk_stbl_cost_set(sp1, blk_stbl_cost_get(cb));
			blk_stbl_cost_set(sp2, blk_stbl_cost_get(cb));
			
			count_splt(cb, x.level);
			
			state_nr.splt_cnt++;
			mrg_lst_add(sp1);
			mrg_lst_add(sp2);
			mrg_lst_rm(cb);
			rels_blk(cb);
			err = 0;
			state_nr.ctt_valid_cnt--;
			state_nr.ptr_valid_cnt -= x.level;
			goto out;
		}
		else{

			count_splt(cb, x.level);
			
			if((err = garbage_collect(sb, &cb, cut)) < 0){
				printk(KERN_INFO "raw_inode_delete: gabage_collec failed...\n");
				goto out;
			}
			raw_inode_read(cb, &x, cut[0].cur);
			goto after_gc;
		}
	}

after_gc:
	blk_obj_num_dec(cb);
	for(i = 0; i < x.level; i++){
		ptrptr_update(cb, cut[i].next, ptrptr_cur_read(cb, x.forward[i], NULL));
	}
	err = 0;
	state_nr.ctt_valid_cnt--;
	state_nr.ptr_valid_cnt -= x.level;
out:
	return err;
}

int garbage_collect(struct sb_info *sb, __u16 *_cb, struct ptrptr* cut){
	__u16 gcb, term, old[MAX_LEVEL], new[MAX_LEVEL], pb, nb, cb = *_cb;
	int i, err = -ENOSPC;
	
	pb = blk_prnt_get(cb);
	nb = blk_next_get(cb);	
	gcb = free_mblk_alloc();

	if(gcb == FH_NULL){
		printk(KERN_INFO "garbage_collect: flash has no space...\n");
		printk(KERN_INFO "do merge_action_1...\n");
		err = merge_action_1(sb, pb, cb, nb);
		if(err < 0)
			goto out;
	}
	
	if((pb != FH_NULL) && (!blk_ptr_spc_chk(pb, 1))){
		printk(KERN_INFO "garbage_collect: no ptrptr pointer in parent block and do pointer scrub\n");
		printk(KERN_INFO "do pointer scrub...\n");
		ptr_scrub(sb, pb);
	}
	
	if(pb == FH_NULL)
		sb->blk_idx = gcb;
	else
		blk_nxt_update(pb, gcb);
	
	blk_init(gcb, pb ,nb);

	if(nb != FH_NULL){
		if(!blk_ptr_spc_chk(nb, 1)){
			printk(KERN_INFO "garbage_collect: next block %d has no pointer for block list..\n", nb);
			printk(KERN_INFO "do pointer scrub...\n");
			ptr_scrub(sb, nb);
		}	
		
		blk_prnt_update(nb, gcb);
	}

	init_cut2thread(old);
	init_cut2thread(new);
	term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
	do_move(sb, cb, gcb, FH_NULL, cut[0].cur, FH_NULL, old, new);
	for(i = 0; i < MAX_LEVEL; i++){
		cut[i].next = new[i];
	}

	do_move(sb, cb, gcb, FH_NULL, FH_NULL, term, old, new);
	for(i = 0; i < MAX_LEVEL; i++){
		cut[i].cur = ptrptr_cur_read(gcb, cut[i].next, NULL);
	}

	mrg_lst_elmt_sbstt(gcb, cb);
	mrg_lst_elmt_reset(cb);
	blk_stbl_cost_inc(gcb);
//	mrg_lst_print(mrg_head.m_start);
	rels_blk(cb);
	*_cb = gcb;
	state_nr.gc_cnt++;
	err = 0;

out:
	return err;
}

void ptr_scrub(struct sb_info *sb, __u16 cb){
	__u16 term, cut[MAX_LEVEL], 
		nb = blk_next_get(cb),
		pb = blk_prnt_get(cb);

	blk_init(sb->tmp_idx, pb, nb); 	/* format reserved block */
	init_cut2thread(cut);
	term = (nb == FH_NULL ) ? FH_NULL : FH_XPRT;
	do_move(sb, cb, sb->tmp_idx, FH_NULL, FH_NULL, term, cut, NULL);  /* move cb to tmp */
	blk_obj_num_set(cb, 0);
	erase_blk(cb);
	blk_init(cb, pb, nb);	/* format cb block */
	init_cut2thread(cut);
	do_move(sb, sb->tmp_idx, cb, FH_NULL, FH_NULL, term, cut, NULL);  /* move tmp to cb */
	blk_obj_num_set(sb->tmp_idx, 0);
	erase_blk(sb->tmp_idx);
	state_nr.scrb_cnt++;
}

void do_merge(struct sb_info *sb, __u16 mb1, __u16 mb2){
	__u16 sp, old[MAX_LEVEL], new[MAX_LEVEL], term,
		nb = blk_next_get(mb2),
		pb = blk_prnt_get(mb1);

	/* parent block need one pointer to link sp1 */
	if((pb != FH_NULL) && !blk_ptr_spc_chk(pb, 1)){
		printk(KERN_INFO "do_merge: parent block %d has no pointer for block list and do pointer scrub\n", pb);
		ptr_scrub(sb, pb);
	}

	init_cut2thread(old);
	init_cut2thread(new);
	blk_init(sb->m1_idx, pb, nb);

	if(pb == FH_NULL)
		sb->blk_idx = sb->m1_idx;
	else
		blk_nxt_update(pb, sb->m1_idx);

	if(nb != FH_NULL){
		if(!blk_ptr_spc_chk(nb, 1)){
			printk(KERN_INFO "do_merge: next block %d has no pointer for block list..\n", nb);
			printk(KERN_INFO "do pointer scrub...\n");
			ptr_scrub(sb, nb);
		}	
		
		blk_prnt_update(nb, sb->m1_idx);
	}

	do_move(sb, mb1, sb->m1_idx, FH_NULL, FH_NULL, FH_NULL, old, new);
	init_cut2thread(old);
	term = (nb != FH_NULL) ? FH_XPRT : FH_NULL;
	do_move(sb, mb2, sb->m1_idx, FH_NULL, FH_NULL, term, old, new);
	mrg_lst[sb->m1_idx].type = BLK_TYPE_META;
	blk_stbl_cost_set(sb->m1_idx, blk_stbl_cost_get(mb1) + blk_stbl_cost_get(mb2));
	mrg_lst_tail_add(sb->m1_idx);
	sb->m1_idx = mb1; /* set mb1 to be the reserved block for merging action */
	mrg_lst_rm(mb1);

	erase_blk(mb1);	/* just erase it to preserved block for merge operations */
	mrg_lst_rm(mb2);
	rels_blk(mb2);	/* release mb2 to free block */
out:
	return;
}

int merge_action_2(struct sb_info *sb, __u16 pb, __u16 cb, __u16 nb){
	__u16 m1, m2;
	int err = -ENOSPC;
	
	if(free_blk_chk(sb, 1)){	/* having one free block */
		m1 = find_mrg_blk_from(FH_NULL, pb, cb, nb);
		if(m1 != FH_NULL){
			do_merge(sb, m1, blk_next_get(m1));
			state_nr.mrg_cnt++;
			err = 0;
		}	
	}
	else{	/* no free block in the system */
		m1 = find_mrg_blk_from(FH_NULL, pb, cb, nb);
		if(m1 != FH_NULL){	/* fist time merge and free one block */
			do_merge(sb, m1, blk_next_get(m1));
			state_nr.mrg_cnt++;
			m2 = find_mrg_blk_from(FH_NULL, pb, cb, nb);
			if(m2 != FH_NULL){	/* second time merge and free another free block */
				do_merge(sb, m2, blk_next_get(m2));
				state_nr.mrg_cnt++;
				err = 0;
			}
		}
	} 
	return err; 
}

int merge_action_1(struct sb_info *sb, __u16 pb, __u16 cb, __u16 nb){
	__u16 m1;
	int err = -ENOSPC;

	m1 = find_mrg_blk_from(FH_NULL, pb, cb, nb);
	if(m1 != FH_NULL){
		state_nr.mrg_cnt++;

		do_merge(sb, m1, blk_next_get(m1));
		err = 0;
	}

	return err;
}


/* raw_inode list operations */
void show_node(struct sb_info *sb){
	struct mdfs_raw_inode x;
	__u16 cb, nb, idx, k;
	__u32 count = 1;

	cb = sb->blk_idx;
	nb = blk_next_get(cb);

	raw_inode_read(cb, &x, ptrptr_cur_read(cb, BLK_ANCHOR_IDX, NULL));
	printk(KERN_INFO " == current block = %u == \n", cb);
	printk(KERN_INFO "{");
	while(1) {
//		printk(KERN_INFO "%u:%u:%s", x.pino, x.ino, x.name);
		printk(KERN_INFO "%u:%s", x.pino, x.name);
		k = ptrptr_cur_read(cb, x.forward[0], NULL);
		if(k == FH_NULL)
			break;
		
		if(is_ptr_export(k)){
			printk(KERN_INFO "}\n");
			printk(KERN_INFO "== Block %u has %u objects\n", cb, count);
			count = 0;
			cb = nb;
			k = ptrptr_cur_read(cb, BLK_ANCHOR_IDX, NULL);
		}

		raw_inode_read(cb, &x, k);
		count++;
		if(cb == nb){
			nb = blk_next_get(cb);
			printk(KERN_INFO " == current block = %u == \n", cb);
			printk(KERN_INFO "{");
		}
		else
			printk(KERN_INFO ", ");
	}
	printk(KERN_INFO "}\n");
	printk(KERN_INFO "\n== Block %u has %u objects\n", cb, count);	
}

void blk_lst_print(struct sb_info *sb){
	__u16 idx = sb->blk_idx;

	printk(KERN_INFO "\t\t==== block list print ==== \t\t\n");
	while(idx != FH_NULL){
		printk(KERN_INFO "B%d->", idx);
//		printk(KERN_INFO "%d %d\n",  mrg_lst[idx].blk_obj_num, mrg_lst[idx].stbl_cost);
		idx = blk_next_get(idx);
	}

	printk(KERN_INFO "FH_NULL\n");
}

void state(struct sb_info *sb){
//	printk(KERN_INFO "\t\t==== state info ==== \t\t\n");
//	printk(KERN_INFO "blk_idx = %u\n", sb->blk_idx);
//	printk(KERN_INFO "m1_idx = %u\n", sb->m1_idx);
//	printk(KERN_INFO "\t\t---- total read/write ctt/ptr ----\n");
	printk(KERN_INFO "total read/write ctt/ptr\n");
	printk(KERN_INFO "ctt_read_cnt = %u\n", state_nr.ctt_read_cnt);
	printk(KERN_INFO "ptr_read_cnt = %u\n", state_nr.ptr_read_cnt);
	printk(KERN_INFO "ctt_write_cnt = %u\n", state_nr.ctt_write_cnt);
	printk(KERN_INFO "ptr_write_cnt = %u\n", state_nr.ptr_write_cnt);
//	printk(KERN_INFO "\t\t---- scrub & gc ----\n");
	printk(KERN_INFO "scrub & gc\n");
//	printk(KERN_INFO "scrub_cnt = %u\n", state_nr.scrb_cnt);
	printk(KERN_INFO "gc_cnt = %u\n", state_nr.gc_cnt);
	printk(KERN_INFO "splt_cnt = %u\n", state_nr.splt_cnt);
//	printk(KERN_INFO "mrg_cnt = %u\n", state_nr.mrg_cnt);
//	printk(KERN_INFO "ctt_split_cnt = %u\n", state_nr.ctt_splt_cnt);
//	printk(KERN_INFO "ptr_split_cnt = %u\n", state_nr.ptr_splt_cnt);
//	printk(KERN_INFO "\t\t---- valid ctt/ptr ----\n");
//	printk(KERN_INFO "valid ctt/ptr\n");
//	printk(KERN_INFO "ctt_valid_cnt = %u\n", state_nr.ctt_valid_cnt);
//	printk(KERN_INFO "ptr_valid_cnt = %u\n", state_nr.ptr_valid_cnt);
//	printk(KERN_INFO "\t\t---- free block----\n");
//	printk(KERN_INFO "free block\n");
//	printk(KERN_INFO "free_blk_cnt = %u\n", state_nr.free_blk_cnt);
	mrg_lst_print(mrg_head.m_start);
	blk_lst_print(sb);
}

struct sb_info *sb_init(void){
	struct sb_info *sb;
	int i;

	sb = (struct sb_info *)kmalloc(sizeof(struct sb_info), GFP_KERNEL);
	sb->blk_idx = HEAD_BLK_IDX; /* record the first block number of the file system list*/ 
	sb->tmp_idx = TEMP_BLK_IDX;
	sb->m1_idx = M1_INIT_IDX;
	sb->m2_idx = M2_INIT_IDX;
	sb->level = MAX_LEVEL;

	return sb;
}

/*
void sbist_remove(struct sb_info *sb){
	struct mdfs_raw_inode *y, *x;
	
	y = x = sb->head->forward[0];
	while(x != NULL) {
		y = x->forward[0];
		kfree(x);
		x = y;
	}

	kfree(sb->head);
	kfree(sb);
}
*/

