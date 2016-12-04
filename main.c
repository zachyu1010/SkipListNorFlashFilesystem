#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flash_drive.h"
#include "mdfs_node.h"
#include "macro.h"
#include "utility.h"
#include "errno.h"

/*
static void rilist_ptr_print(struct sb_info *ril){
	struct mdfs_raw_inode *x = ril->head;

	printf("\n");
	for(; x != NULL; x = x->forward[0]){
		int i;
		printf("(%s, %d, %p)\n", x->name, x->level, x);
		for(i = 0; i < MAX_LEVEL; i++){
			printf("%10p ", x->forward[i]);
		}
		printf("\n");
	}
}
*/
static struct mdfs_raw_inode *alloc_set_raw_inode(unsigned int pino, unsigned int ino, char *name){
	struct mdfs_raw_inode *x;
	int lvl;

//	lvl = random_level();
//printf("level = %d\n", lvl);
	x = raw_inode_alloc();
	x->ino = ino;
	x->pino = pino;
	x->dsize = 0;
	strcpy((char *)x->name, name);
	x->nsize = strlen(name);

	return x;
}

#define PHY_START	(0)	
#define WR_LEN 		(BLK_CTT_BMP_ALLOT_SIZE-1)
extern __u8 flash_blk_bmp[TOTAL_BLK_NUM >> 3];

__u32 random_ino(){
//	return rand32();
	return rand();
}

int main(){
	unsigned char block[WR_LEN];
	size_t retlen;
	struct mdfs_raw_inode *mri;
	struct sb_info *sb;
	__u32 i,j, k;
	char name[20];
    FILE *fd;
	
	init_drive();
	format_drive();
	init_core_struct();
	sb = sb_init();

#define TEST1 0 /* seq insrt TEST_NUM */
#define TEST2 0 /* seq insrt 4 & seq insrt 3 */
#define TEST3 0 /* seq insrt TEST_NUM & seq updt TEST_NUM */
#define TEST4 0 /* seq insrt TEST_NUM & seq del TEST_NUM */
#define TEST5 0 /* rand insrt TEST_NUM */
#define TEST6 0 /* seq insrt TEST_NUM & rand updt TEST_NUM */
#define TEST7 0	/* seq insrt TEST_NUM & rand del TEST_NUM */
#define TEST8 1 /* seq insrt TEST_NUM & normal distribute updates */

#define TEST_NUM 20000//(BLK_INODE_NUM * 7)

/*test1*/
/***********************************************************************************
Sequentially insert TEST_NUM of objects
***********************************************************************************/
#if TEST1
	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}
#endif

/*test2*/
/***********************************************************************************
Sequentially insert INODE_BLOCK_NUM*4 of objects ( objects of even name )
Sequentially insert INODE_BLOCK_NUM*3 of objects ( objects of odd name)
***********************************************************************************/
#if TEST2
	for(i = 0; i < BLK_INODE_NUM * 4; i++){
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}


	for(i = 0; i < BLK_INODE_NUM * 3; i++){
		sprintf(name, "%010d", i*2 + 1);
		mri = alloc_set_raw_inode(1, i*2, name);
		if(raw_inode_insert(sb, mri) < 0){
			printf("main: insert %s failed...\n", name);
			free(mri);
			break;
		}

		free(mri);
	}
#endif


/*test3*/
/***********************************************************************************
Sequentially insert TEST_NUM of objects
Sequentially update TEST_NUM of objects
***********************************************************************************/
#if TEST3

	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}

	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d",  i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_update(sb, mri);
		free(mri);
	}
#endif

/*test4*/
/***********************************************************************************
Sequentially insert TEST_NUM of objects
Sequentially delete TEST_NUM of objects
***********************************************************************************/
#if TEST4
	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}

	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d", i*2);
		raw_inode_delete(sb, 1, name);
	}	
#endif

/*test5*/
/***********************************************************************************
Randomly insert TEST_NUM of objects
***********************************************************************************/
#if TEST5
	for(i = 0; i < TEST_NUM; i++){
		j = random_ino();
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(j, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}
#endif

/*test6*/
/***********************************************************************************
Sequentially insert TEST_NUM of objects
Randomly update TEST_NUM of objects
***********************************************************************************/
#if TEST6
	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}

	k = 0;
	while(1){
		j = random_ino() % TEST_NUM;
		sprintf(name, "%010d", j*2);
		mri = alloc_set_raw_inode(1, j*2, name);
		k++;
		if(raw_inode_update(sb, mri) < 0){
			printf("main: update %s failed...\n", name);
			free(mri);
			break;
		}
		
		if(k >= TEST_NUM)
			break;
	}

	printf("k = %d\n", k);
#endif

/*test7*/
/***********************************************************************************
Sequentially insert TEST_NUM of objects
Randomly delete INODE_TEST_NUM of objects
***********************************************************************************/
#if TEST7
	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%010d", i*2);
		mri = alloc_set_raw_inode(1, i*2, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}

	
	k = 0;
	while(1){
		__u32 rctt, rptr;
		
		j = random_ino() % TEST_NUM;
		sprintf(name, "%010d", j * 2);
		rctt = state_nr.ctt_read_cnt;
		rptr = state_nr.ptr_read_cnt;
		if(raw_inode_delete(sb, 1, name) == 0){
			k++;
			if(k >= TEST_NUM)
				break;
		}
		else{
			state_nr.ctt_read_cnt = rctt;
			state_nr.ptr_read_cnt = rptr;
		}
	}
#endif

/*test8*/
/***********************************************************************************
Sequentially insert TEST_NUM of objects
update INODE_TEST_NUM of objects by normal distribution
***********************************************************************************/
#if TEST8
	for(i = 0; i < TEST_NUM; i++){
		sprintf(name, "%10d", i);
		mri = alloc_set_raw_inode(1, i, name);
		raw_inode_insert(sb, mri);
		free(mri);
	}

	fd = fopen("p100.log", "r");
	while(1){
		k = fscanf(fd, "%d\n", &j);
		sprintf(name, "%10d", j);
//		printf("name = %s", name);
		if(k == 0 || k == EOF)
			break;

		mri = alloc_set_raw_inode(1, 0, name);
		raw_inode_update(sb, mri);
	}

#endif

out:
	flash_dump();
//	show_node(sb);
	state(sb);
	fsck(sb);
	return 0;
}


