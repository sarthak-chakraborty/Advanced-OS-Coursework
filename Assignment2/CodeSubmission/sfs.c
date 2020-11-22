
#include "sfs.h"


#define MOUNTED 1
#define UNMOUNTED 0

typedef unsigned char* bitmap_t;

int STATE = MOUNTED;
disk *mem_diskptr = NULL;
void* temp_block;

typedef struct bitmap_m {
	bitmap_t bitmap_inode;
	bitmap_t bitmap_data;
} bitmap_m;
bitmap_m mem_bitmap;


void set_bitmap(bitmap_t b, int i) {
	b[i / 8] |= 1 << (i & 7);
}

void unset_bitmap(bitmap_t b, int i) {
	b[i / 8] &= ~(1 << (i & 7));
}

int get_bitmap(bitmap_t b, int i) {
	return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}


inode* retrieve_inode(super_block* sb, int inumber) {
	int iblock = ceil(inumber / 8), inum_in_block = inumber & 7;

	inode *node = NULL;
	// bitmap_t bitmap = mem_diskptr->block_arr[sb->inode_bitmap_block_idx];
	if (get_bitmap(mem_bitmap.bitmap_inode, inumber))
		node = (inode*)(mem_diskptr->block_arr[iblock + sb->inode_block_idx] + inum_in_block * BLOCKSIZE);
	return node;
}


int format(disk *diskptr) {
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
	// setting up a temp block for retrieving all blocks temporarily
	temp_block = (void*)malloc(BLOCKSIZE);

	ret = write_block(diskptr, 0, &sb);
	if (ret == -1) {
		printf("[ERROR] __Super Block writing failed__\n [ERROR] __Format disk failed__\n\n");
		return -1;
	}


	/* Initialize Inode Bitmap */
	bitmap_t bitmap_i = (bitmap_t)malloc((sb.inodes + 7) / 8);
	for (int i = 0; i < sb.inodes; i++)
		unset_bitmap(bitmap_i, i);

	if (IB == 1) {
		ret = write_block(diskptr, sb.inode_bitmap_block_idx, bitmap_i);
		if (ret == -1) {
			printf("[ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}
	else {
		uint32_t start_inode_bitmap_block_ids = sb.inode_bitmap_block_idx;
		for (int i = 0 ; i < IB - 1; i++) {
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (bitmap_i + i * BLOCKSIZE), BLOCKSIZE);

			ret = write_block(diskptr, start_inode_bitmap_block_ids, bitmap_proxy);
			if (ret == -1) {
				printf("[ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
				return -1;
			}

			free(bitmap_proxy);
			start_inode_bitmap_block_ids++;
		}

		ret = write_block(diskptr, start_inode_bitmap_block_ids, (bitmap_i + (IB - 1) * BLOCKSIZE));
		if (ret == -1) {
			printf("[ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}


	/* Initialize Data Bitmap */
	bitmap_t bitmap_d = (bitmap_t)malloc((sb.data_blocks + 7) / 8);
	for (int i = 0; i < sb.data_blocks; i++)
		unset_bitmap(bitmap_d, i);

	if (DBB == 1) {
		ret = write_block(diskptr, sb.data_block_bitmap_idx, bitmap_d);
		if (ret == -1) {
			printf("[ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}
	else {
		uint32_t start_data_bitmap_block_ids = sb.data_block_bitmap_idx;
		for (int i = 0 ; i < DBB - 1; i++) {
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (bitmap_d + i * BLOCKSIZE), BLOCKSIZE);

			ret = write_block(diskptr, start_data_bitmap_block_ids, bitmap_proxy);
			if (ret == -1) {
				printf("[ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
				return -1;
			}

			free(bitmap_proxy);
			start_data_bitmap_block_ids++;
		}

		ret = write_block(diskptr, start_data_bitmap_block_ids, (bitmap_d + (DBB - 1) * BLOCKSIZE));
		if (ret == -1) {
			printf("[ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}


	/* Initialize inode with valid = 0 */
	for (int block_idx = 0; block_idx < I; block_idx++) {
		inode* nodes_in_block = (inode *)malloc(128 * sizeof(inode));

		for (int inode_idx = 0; inode_idx < 128; inode_idx++) {
			inode node = {
				.valid = 0,
				.size = -1,
				.direct = { -1, -1, -1, -1, -1},
				.indirect = -1
			};
			nodes_in_block[inode_idx] = node;
		}
		ret = write_block(diskptr, sb.inode_block_idx + block_idx, nodes_in_block);
		if (ret == -1) {
			printf("[ERROR] __Inode initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}

	STATE = UNMOUNTED;
	return 0;
}


int mount(disk *diskptr) {
	int ret;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("@create_file reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));



	if (sb.magic_number != MAGIC) {
		printf("[ERROR] __Magic Number verification failed__\n [ERROR] __Mounting the file system failed__\n\n");
		return -1;
	}

	/*
	TODO: load bitmaps and mounted file descriptor in the memory
	*/
	int inode_bitmap_block_start = sb.inode_bitmap_block_idx;
	int inode_bitmap_block_end = sb.data_block_bitmap_idx - 1;

	mem_bitmap.bitmap_inode = (bitmap_t)malloc((sb.inodes + 7) / 8);

	if (inode_bitmap_block_start == inode_bitmap_block_end) {
		ret = read_block(mem_diskptr, inode_bitmap_block_start, mem_bitmap.bitmap_inode);
		if (ret == -1) {
			printf("[ERROR] __Create File failed__\n [ERROR] __Disk read for Inode Bitmap failed__\n\n");
			return -1;
		}
	}
	else {
		for (int i = inode_bitmap_block_start; i < inode_bitmap_block_end; i++) {
			int block_addr = i - inode_bitmap_block_start;
			ret = read_block(mem_diskptr, i, (mem_bitmap.bitmap_inode + block_addr * BLOCKSIZE));
			if (ret == -1) {
				printf("[ERROR] __Create File failed__\n [ERROR] __Disk read for Inode Bitmap failed__\n\n");
				return -1;
			}
		}
	}

	mem_diskptr = diskptr;
	STATE = MOUNTED;

	return 0;
}


int create_file() {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[ERROR] __Create File failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}
	int ret;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("@create_file reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));

	/* Find the leftmost unset bit */
	int free_inode_pos = -1;
	for (int i = 0; i < sb.inodes; i++) {
		if (get_bitmap(mem_bitmap.bitmap_inode, i)) {
			free_inode_pos = i;
			break;
		}
	}

	/* Update Inode Informtion */
	inode *node = retrieve_inode(&sb, free_inode_pos);
	if (node == NULL) {
		printf("[ERROR] __Create File failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}

	node = NULL;
	int iblock = ceil(free_inode_pos / 8), inum_in_block = free_inode_pos & 7;
	// bitmap_t bitmap = mem_diskptr->block_arr[sb->inode_bitmap_block_idx];
	if (get_bitmap(mem_bitmap.bitmap_inode, free_inode_pos))
		node = (inode*)(mem_diskptr->block_arr[iblock + sb.inode_block_idx] + inum_in_block * BLOCKSIZE);
	// return node;
	node->size = 0;
	node->valid = 1;
	set_bitmap(mem_bitmap.bitmap_inode, free_inode_pos);

	// memcpy(mem_diskptr->block_arr[iblock + sb.inode_block_idx] + inum_in_block * BLOCKSIZE, node, sizeof(*node));


	/* Update Inode Bitmap */
	int inode_bitmap_block_start = sb.inode_bitmap_block_idx;
	int inode_bitmap_block_end = sb.data_block_bitmap_idx - 1;

	if (inode_bitmap_block_start == inode_bitmap_block_end) {
		ret = write_block(mem_diskptr, sb.inode_bitmap_block_idx, mem_bitmap.bitmap_inode);
		if (ret == -1) {
			printf("[ERROR] __Create File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
			return -1;
		}
	}
	else {
		for (int i = inode_bitmap_block_start; i < inode_bitmap_block_end; i++) {
			int block_addr = i - inode_bitmap_block_start;
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (mem_bitmap.bitmap_inode + block_addr * BLOCKSIZE), BLOCKSIZE);

			ret = write_block(mem_diskptr, i, bitmap_proxy);
			if (ret == -1) {
				printf("[ERROR] __Create File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
				return -1;
			}
			free(bitmap_proxy);
		}
	}

	return free_inode_pos;
}


int remove_file(int inumber) {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[ERROR] __Remove File failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}

	if (!get_bitmap(mem_bitmap.bitmap_inode, inumber)) {
		printf("[ERROR] __Remove File failed__\n [ERROR] __Inode number is not set__\n\n");
		return -1;
	}

	int ret;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("@remove_file reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));

	/* Update Inode Informtion */
	inode *node = retrieve_inode(&sb, inumber);
	if (node == NULL) {
		printf("[ERROR] __Remove File failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}
	node->valid = 0;
	unset_bitmap(mem_bitmap.bitmap_inode, inumber);

	int iblock = ceil(inumber / 8), inum_in_block = inumber & 7;
	// memcpy(mem_diskptr->block_arr[iblock + sb.inode_block_idx] + inum_in_block * BLOCKSIZE, node, sizeof(*node));

	/* Update Inode Bitmap */
	int inode_bitmap_block_start = sb.inode_bitmap_block_idx;
	int inode_bitmap_block_end = sb.data_block_bitmap_idx - 1;

	if (inode_bitmap_block_start == inode_bitmap_block_end) {
		ret = write_block(mem_diskptr, sb.inode_bitmap_block_idx, mem_bitmap.bitmap_inode);
		if (ret == -1) {
			printf("[ERROR] __Remove File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
			return -1;
		}
	}
	else {
		for (int i = inode_bitmap_block_start; i < inode_bitmap_block_end; i++) {
			int block_addr = i - inode_bitmap_block_start;
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (mem_bitmap.bitmap_inode + block_addr * BLOCKSIZE), BLOCKSIZE);

			ret = write_block(mem_diskptr, i, bitmap_proxy);
			if (ret == -1) {
				printf("[ERROR] __Remove File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
				return -1;
			}
			free(bitmap_proxy);
		}
	}

	return 0;
}


int stat(int inumer) {
	int ret;

	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("@stat reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));


	inode *node = retrieve_inode(&sb, inumer);
	if (node == NULL) {
		printf("[ERROR] __Print Stat failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}
	uint32_t size = node->size;
	uint32_t num_direct_pointers = (size > 5 * BLOCKSIZE) ? 5 : ceil(size / BLOCKSIZE);
	uint32_t num_indirect_pointers = (size > 5 * BLOCKSIZE) ? ceil((size - 5 * BLOCKSIZE) / BLOCKSIZE) : 0;
	uint32_t num_data_blocks = (num_indirect_pointers == 0) ? num_direct_pointers : num_direct_pointers + num_indirect_pointers + 1;

	printf("[SUCCESS] __Inode Information fetched successfully__\n");
	printf("Logical Size: %d\n", size);
	printf("Number of Data Blocks in Use: %d\n", num_data_blocks);
	printf("Number of direct pointers: %d", num_direct_pointers);
	printf("Number of indirect pointers: %d", num_indirect_pointers);

	return 0;
}


char* retrieve_data_block(super_block* sb, int block_idx) {

}


int read_i(int inumber, char *data, int length, int offset) {
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("@read_i reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));
	//validating inode number
	if (inumber < 0 || inumber >= sb.inodes) {
		return -1;
	}
	//validating length
	if (length < 0);
	//validating offset
	/*
	load super_block into &sb
	load inode_bitmap into bitmap
	check if the inode is present by bitmap[inumber] == 1
	blocknum = (sb.first_inode_block + inumber)/8, offset_inside_blocknum = (same cheez)%8
	retirieve that block with blocknum
	retrieve that inode from this block
	for offset = givenoffset; keep running:
	    if offset is < 5*BLOCK_SIZE
	        retrieve that direct block
	    else
	         retrieve that indirect indirect pointer and usme retrieve data block
	    from the retrieved data block given offset read
	    toread = BLOCK_SIZE;
	    if( length <= BLOCK_SIZE ){
	        toread = length;
	    }
	    update the offset by offset += toread
	    length -= toread
	*/


	// mem_bitmap = {sb.};
}


int write_i(int inumber, char *data, int length, int offset);