#include "disk.h"
#include "sfs.h"

#define MOUNTED 1
#define UNMOUNTED 0

typedef unsigned char* bitmap_t;

int STATE = MOUNTED;
typedef struct mem_bitmap{
	bitmap_t bitmap_inode;
	bitmap_t bitmap_data;
} mem_bitmap;


void set_bitmap(bitmap_t b, int i) {
    b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
    b[i / 8] &= ~(1 << (i & 7));
}

int get_bitmap(bitmap_t b, int i) {
    return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}


int format(disk *diskptr){
	super_block sb;
	int M, N, I, IB, R, DBB, DB;
	int ret;

	N = diskptr->blocks;
	M = N - 1;
	I = floor(0.1 * M);
	IB = ceil((I * 128) / (8 * BLOCKSIZE));
	R = M - I - IB;
	DBB = ceil(R / (8 * BLOCKSIZE));
	DB = R - DBB;

	/* Initialize Super Block */
	sb.magic_number = 12345;
	sb.blocks = M;
	sb.inode_blocks = I;
	sb.inodes = I * 128;
	sb.inode_bitmap_block_idx = 1;
	sb.inode_block_idx = 1 + IB + DBB;
	sb.data_block_bitmap_idx = 1 + IB;
	sb.data_block_idx = 1 + IB + DBB + I;
	sb.data_blocks = DB;

	ret = write_block(diskptr, 0, &sb);
	if(ret == -1){
		printf("[ERROR] __Super Block writing failed__\n [ERROR] __Format disk failed__\n\n");
		return -1;
	}


	/* Initialize Inode Bitmap */
	bitmap_t bitmap_i = (bitmap_t)malloc((sb.inodes + 7) / 8);
	for(int i = 0; i < sb.inodes; i++)
		unset_bitmap(bitmap_i, i);

	if(IB == 1){
		ret = write_block(diskptr, sb.inode_bitmap_block_idx, bitmap_i);
		if(ret == -1){
			printf("[ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}
	else{
		uint32_t start_inode_bitmap_block_ids = sb.inode_bitmap_block_idx;
		for(int i = 0 ; i < IB-1; i++){
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (bitmap_i + i*BLOCKSIZE), BLOCKSIZE);

			ret = write_block(diskptr, start_inode_bitmap_block_ids, bitmap_proxy);
			if(ret == -1){
				printf("[ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
				return -1;
			}

			free(bitmap_proxy);
			start_inode_bitmap_block_ids++;
		}

		ret = write_block(diskptr, start_inode_bitmap_block_ids, (bitmap_i + (IB-1)*BLOCKSIZE));
		if(ret == -1){
			printf("[ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}


	/* Initialize Data Bitmap */
	bitmap_t bitmap_d = (bitmap_t)malloc((sb.data_blocks + 7) / 8);
	for(int i = 0; i < sb.data_blocks; i++)
		unset_bitmap(bitmap_d, i);

	if(DBB == 1){
		ret = write_block(diskptr, sb.data_block_bitmap_idx, bitmap);
		if(ret == -1){
			printf("[ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}
	else{
		uint32_t start_data_bitmap_block_ids = sb.data_block_bitmap_idx;
		for(int i = 0 ; i < DBB-1; i++){
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (bitmap_d + i*BLOCKSIZE), BLOCKSIZE);

			ret = write_block(diskptr, start_data_bitmap_block_ids, bitmap_proxy);
			if(ret == -1){
				printf("[ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
				return -1;
			}

			free(bitmap_proxy);
			start_data_bitmap_block_ids++;
		}

		ret = write_block(diskptr, start_data_bitmap_block_ids, (bitmap_d + (DBB-1)*BLOCKSIZE));
		if(ret == -1){
			printf("[ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}


	/* Initialize inode with valid = 0 */
	for(int block_idx = 0; block_idx < I; block_idx++){
		inode* nodes_in_block = (inode *)malloc(128 * sizeof(inode));

		for(int inode_idx = 0; inode_idx < 128; inode_idx++){
			inode node = {
				.valid = 0,
				.size = -1,
				.direct = {-1, -1, -1, -1, -1},
				.indirect = -1
			};
			nodes_in_block[inode_idx] = inode;
		}
		ret = write_block(diskptr, sb.inode_block_idx + i, nodes_in_block);
		if(ret == -1){
			printf("[ERROR] __Inode initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}

	return 0;
}


int mount(disk *diskptr){
	int ret;

	super_block *sb;
	sb = (super_block *)malloc(sizeof(super_block));

	ret = read_block(diskptr, 0, sb);
	if(ret == -1){
		printf("[ERROR] __Super block read failed__\n [ERROR] __Mounting the file system failed__\n\n");
		return -1;
	}

	if(sb->magic_number != MAGIC){
		printf("[ERROR] __Magic Number verification failed__\n [ERROR] __Mounting the file system failed__\n\n");
		return -1;
	}

	/*
	TODO: load bitmaps and mounted file descriptor in the memory
	*/

	return 0;
}