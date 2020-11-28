/*Assignment 2
------------------------------------------
Sankalp R. 16CS30031
Sarthak Charkraborty 16CS30044
------------------------------------------
*/
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


int max(int x, int y) {
	if (x > y)return x;
	return y;
}
int min(int x, int y) {
	if (x < y)return x;
	return y;
}
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
	int iblock = floor((inumber) / (BLOCKSIZE / 8)), offset = (inumber) % (BLOCKSIZE / 8);

	inode *node = (inode*)malloc(sizeof(inode));
	read_block(mem_diskptr, (iblock) + sb->inode_block_idx, temp_block);
	memcpy(node, temp_block + offset * 8, sizeof(inode));
	return node;
}


int write_inode(super_block* sb, int inumber, inode *node) {
	int iblock = floor((inumber) / (BLOCKSIZE / 8)), offset = (inumber) % (BLOCKSIZE / 8);
	read_block(mem_diskptr, iblock, temp_block);
	memcpy(temp_block + offset * 8, node, sizeof(inode));
	printf("[sfs] @write_inode after write_i iblock(%d) + sb->inode_block_idx(%d)\n", iblock, sb->inode_block_idx);
	if (write_block(mem_diskptr, iblock + sb->inode_block_idx, temp_block) < 0) {
		printf("[sfs] ERROR @write_inode writing inode to disk\n");
		return -1;
	}
	return 0;
}


int format(disk *diskptr) {
	super_block sb;
	int M, N, I, IB, R, DBB, DB;
	int ret;

	N = diskptr->blocks;
	M = N - 1;
	I = floor(0.1 * M);
	IB = ceil((double)(I * 128) / (8 * BLOCKSIZE));
	R = M - I - IB;
	DBB = ceil((double)R / (8 * BLOCKSIZE));
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
	printf("[sfs] N : %d M: %d, I:%d, IB:%d, DBB:%d, DB:%d\n", N, M, I, IB, DBB, DB);
	printf("[sfs] first_idx >> inode_bitmp : %d, inode: %d, data_bitmp:%d, data:%d\n",
	       sb.inode_bitmap_block_idx, sb.inode_block_idx, sb.data_block_bitmap_idx, sb.data_block_idx);
	// setting up a temp block for retrieving all blocks temporarily
	temp_block = (void*)malloc(BLOCKSIZE);

	ret = write_block(diskptr, 0, &sb);
	if (ret == -1) {
		printf("[sfs] [ERROR] __Super Block writing failed__\n [ERROR] __Format disk failed__\n\n");
		return -1;
	}


	/* Initialize Inode Bitmap */
	bitmap_t bitmap_i = (bitmap_t)malloc((sb.inodes + 7) / 8);
	for (int i = 0; i < sb.inodes; i++)
		unset_bitmap(bitmap_i, i);

	if (IB == 1) {
		ret = write_block(diskptr, sb.inode_bitmap_block_idx, bitmap_i);
		if (ret == -1) {
			printf("[sfs] [ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
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
				printf("[sfs] [ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
				return -1;
			}

			free(bitmap_proxy);
			start_inode_bitmap_block_ids++;
		}

		ret = write_block(diskptr, start_inode_bitmap_block_ids, (bitmap_i + (IB - 1) * BLOCKSIZE));
		if (ret == -1) {
			printf("[sfs] [ERROR] __Inode Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
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
			printf("[sfs] [ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
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
				printf("[sfs] [ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
				return -1;
			}

			free(bitmap_proxy);
			start_data_bitmap_block_ids++;
		}

		ret = write_block(diskptr, start_data_bitmap_block_ids, (bitmap_d + (DBB - 1) * BLOCKSIZE));
		if (ret == -1) {
			printf("[sfs] [ERROR] __Data Bitmap initialization failed__\n [ERROR] __Format disk failed__\n\n");
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
			printf("[sfs] [ERROR] __Inode initialization failed__\n [ERROR] __Format disk failed__\n\n");
			return -1;
		}
	}

	STATE = UNMOUNTED;
	return 0;
}


int mount(disk *diskptr) {
	int ret;
	mem_diskptr = diskptr;
	// printf("[sfs] @mount hello0\n");
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @create_file reading super_block ERROR\n");
		return -1; //error in read
	}
	// printf("[sfs] @mount hello1\n");
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));
	// printf("[sfs] @mount hello2\n");


	if (sb.magic_number != MAGIC) {
		printf("[sfs] [ERROR] __Magic Number verification failed__\n [ERROR] __Mounting the file system failed__\n\n");
		return -1;
	}

	/*
	Load Inode Bitmap in the memory
	*/
	int inode_bitmap_block_start = sb.inode_bitmap_block_idx;
	int inode_bitmap_block_end = sb.data_block_bitmap_idx - 1;

	mem_bitmap.bitmap_inode = (bitmap_t)malloc((sb.inodes + 7) / 8);

	if (inode_bitmap_block_start == inode_bitmap_block_end) {
		ret = read_block(mem_diskptr, inode_bitmap_block_start, mem_bitmap.bitmap_inode);
		if (ret == -1) {
			printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Disk read for Inode Bitmap failed__\n\n");
			return -1;
		}
	}
	else {
		for (int i = inode_bitmap_block_start; i < inode_bitmap_block_end; i++) {
			int block_addr = i - inode_bitmap_block_start;
			ret = read_block(mem_diskptr, i, (mem_bitmap.bitmap_inode + block_addr * BLOCKSIZE));
			if (ret == -1) {
				printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Disk read for Inode Bitmap failed__\n\n");
				return -1;
			}
		}
	}

	/*
	Load Data Bitmap in the memory
	*/
	int data_bitmap_block_start = sb.data_block_bitmap_idx;
	int data_bitmap_block_end = sb.inode_block_idx - 1;

	mem_bitmap.bitmap_data = (bitmap_t)malloc((sb.data_blocks + 7) / 8);

	if (data_bitmap_block_start == data_bitmap_block_end) {
		ret = read_block(mem_diskptr, data_bitmap_block_start, mem_bitmap.bitmap_data);
		if (ret == -1) {
			printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Disk read for Data Bitmap failed__\n\n");
			return -1;
		}
	}
	else {
		for (int i = data_bitmap_block_start; i < data_bitmap_block_end; i++) {
			int block_addr = i - data_bitmap_block_start;
			ret = read_block(mem_diskptr, i, (mem_bitmap.bitmap_data + block_addr * BLOCKSIZE));
			if (ret == -1) {
				printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Disk read for Data Bitmap failed__\n\n");
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
		printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}
	int ret;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @create_file reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));

	/* Find the leftmost unset bit */
	int free_inode_pos = -1;
	printf("[sfs] sb.indodes = %d\n", sb.inodes);
	for (int i = 0; i < sb.inodes; i++) {
		printf("[sfs] @create_file value of get_bitmap : %d\n", get_bitmap(mem_bitmap.bitmap_inode, i));
		if (!get_bitmap(mem_bitmap.bitmap_inode, i)) {
			free_inode_pos = i;
			break;
		}
	}

	/* Update Inode Informtion */
	inode *node = retrieve_inode(&sb, free_inode_pos);
	if (node == NULL) {
		printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}

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
			printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
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
				printf("[sfs] [ERROR] __Create File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
				return -1;
			}
			free(bitmap_proxy);
		}
	}
	printf("[sfs] create file is a SUCESS\n");
	write_inode(&sb, free_inode_pos, node);
	return free_inode_pos;
}


int remove_file(int inumber) {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}

	if (!get_bitmap(mem_bitmap.bitmap_inode, inumber)) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Inode number is not set__\n\n");
		return -1;
	}

	int ret;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @remove_file reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));

	/* Update Inode Informtion */
	inode *node = retrieve_inode(&sb, inumber);
	if (node == NULL) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}
	node->valid = 0;
	unset_bitmap(mem_bitmap.bitmap_inode, inumber);

	/* Unset Data Bitmap */
	if (node->size > 0) {
		int end_idx = node->size / BLOCKSIZE, start_idx = 0;
		if (node->size < 5 * BLOCKSIZE) {
			for (; start_idx <= min(end_idx, 4); start_idx++) {
				unset_bitmap(mem_bitmap.bitmap_data, node->direct[start_idx]);
			}
		}
		start_idx -= 5;
		if (start_idx >= 0) {
			DECL_BLOCK(indirect);
			if (read_block(mem_diskptr, node->indirect, indirect) < 0) {
				printf("[sfs] [ERROR] @remove_file read_block error (node->indirect=%d)\n", node->indirect );
			}
			int* ptr = indirect;
			ptr += start_idx;
			for (; ((void*)ptr < indirect + (node->size - 5 * BLOCKSIZE)); ptr++) {
				unset_bitmap(mem_bitmap.bitmap_data, *ptr);
			}

			FREE_BLOCK(indirect);
		}
	}

	int iblock = ceil(inumber / 8), inum_in_block = inumber & 7;
	// memcpy(mem_diskptr->block_arr[iblock + sb.inode_block_idx] + inum_in_block * BLOCKSIZE, node, sizeof(*node));

	/* Update Inode Bitmap */
	int inode_bitmap_block_start = sb.inode_bitmap_block_idx;
	int inode_bitmap_block_end = sb.data_block_bitmap_idx - 1;

	if (inode_bitmap_block_start == inode_bitmap_block_end) {
		ret = write_block(mem_diskptr, sb.inode_bitmap_block_idx, mem_bitmap.bitmap_inode);
		if (ret == -1) {
			printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
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
				printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Inode Bitmap Write failed__\n\n");
				return -1;
			}
			free(bitmap_proxy);
		}
	}


	/* Update Data Bitmap */
	int data_bitmap_block_start = sb.data_block_bitmap_idx;
	int data_bitmap_block_end = sb.inode_block_idx - 1;

	if (data_bitmap_block_start == data_bitmap_block_end) {
		ret = write_block(mem_diskptr, sb.data_block_bitmap_idx, mem_bitmap.bitmap_data);
		if (ret == -1) {
			printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Data Bitmap Write failed__\n\n");
			return -1;
		}
	}
	else {
		for (int i = data_bitmap_block_start; i < data_bitmap_block_end; i++) {
			int block_addr = i - data_bitmap_block_start;
			bitmap_t bitmap_proxy = (bitmap_t)malloc(BLOCKSIZE);
			memcpy(bitmap_proxy, (mem_bitmap.bitmap_data + block_addr * BLOCKSIZE), BLOCKSIZE);

			ret = write_block(mem_diskptr, i, bitmap_proxy);
			if (ret == -1) {
				printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Data Bitmap Write failed__\n\n");
				return -1;
			}
			free(bitmap_proxy);
		}
	}
	write_inode(&sb, inumber, node);
	free(node);
	return 0;
}


int stat(int inumer) {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[sfs] [ERROR] __Return Stat failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}

	if (!get_bitmap(mem_bitmap.bitmap_inode, inumer)) {
		printf("[sfs] [ERROR] __Return Stat failed__\n [ERROR] __Inode number is not set__\n\n");
		return -1;
	}

	int ret;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @stat reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));


	inode *node = retrieve_inode(&sb, inumer);
	if (node == NULL) {
		printf("[sfs] [ERROR] __Print Stat failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}
	uint32_t size = node->size;
	uint32_t num_direct_pointers = (size > 5 * BLOCKSIZE) ? 5 : ceil(size / BLOCKSIZE);
	uint32_t num_indirect_pointers = (size > 5 * BLOCKSIZE) ? ceil((size - 5 * BLOCKSIZE) / BLOCKSIZE) : 0;
	uint32_t num_data_blocks = (num_indirect_pointers == 0) ? num_direct_pointers : num_direct_pointers + num_indirect_pointers + 1;

	printf("[sfs] [SUCCESS] __Inode Information fetched successfully__\n");
	printf("[sfs] Logical Size: %d\n", size);
	printf("[sfs] Number of Data Blocks in Use: %d\n", num_data_blocks);
	printf("[sfs] Number of direct pointers: %d", num_direct_pointers);
	printf("[sfs] Number of indirect pointers: %d", num_indirect_pointers);

	return 0;
}



int read_i(int inumber, char *data, int length, int offset) {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}

	if (!get_bitmap(mem_bitmap.bitmap_inode, inumber)) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Inode number is not set__\n\n");
		return -1;
	}
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @read_i reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));

	//validating inode number
	if (inumber < 0 || inumber >= sb.inodes || length < 0 || offset < 0 || offset > 1029 * BLOCKSIZE) {
		printf("[sfs] [ERROR] __Read_i failed__\n [ERROR] __Invalid Arguments__\n\n");
		return -1;
	}

	//validating inode
	inode *node = retrieve_inode(&sb, inumber);
	if (node == NULL) {
		printf("[sfs] [ERROR] __Read_i failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}
	if (offset > node->size) {
		printf("[sfs] [ERROR] __Read_i failed__\n [ERROR] __Offset greater than size__\n\n");
		return -1;
	}
	if (node->valid == 0) {
		printf("[sfs] [ERROR] @read_i file doesn't contain requested amount of data\n");
		return -1;
	}
	if (node->size - offset < length) { // requested length is greater than data present after offset
		length = node->size - offset;
	}
	int total_data_read = 0;
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

	printf("Inode_size:%d, inode->valid:%d, inode->direct[0]:%d, inode->direct[1]:%d\n",
	       node->size, node->valid, node->direct[0], node->direct[1]);
	int start_idx = offset / BLOCKSIZE, block_offset = offset % BLOCKSIZE, bytes_toread;
	if (offset < 5 * BLOCKSIZE) {
		for (; length && start_idx < 5; start_idx++) {
			bytes_toread = BLOCKSIZE - block_offset;
			if (length < bytes_toread) {
				bytes_toread = length;
			}
			if (read_block(mem_diskptr, node->direct[start_idx], temp_block) < 0) {
				printf("[sfs] @read_i reading from direct block (node->direct[%d] = %d) ERROR\n", start_idx, node->direct[start_idx]);
				return -1; //error in read
			}
			memcpy(data, temp_block + block_offset, bytes_toread);
			printf("block_offset:%d, bytes_toread:%d\n", block_offset, bytes_toread);
			total_data_read += bytes_toread;
			data += bytes_toread;
			length -= bytes_toread;
			block_offset = 0;
		}
	}
	else {
		start_idx -= 5;
	}
	if (start_idx >= 0 && length > 0) {
		DECL_BLOCK(indirect);
		read_block(mem_diskptr, node->indirect, indirect);
		int* ptr = indirect;
		ptr += start_idx;
		for (; length && ((void*)ptr < indirect + (node->size - 5 * BLOCKSIZE)); ptr++) {
			bytes_toread = BLOCKSIZE - block_offset;
			if (length < bytes_toread) {
				bytes_toread = length;
			}
			if (mem_bitmap.bitmap_data[*ptr] == 0) {
				printf("[sfs] GRAVE INCONSISTENCIES!!! some of the blocks should \
					have had their bit set according to the size of file inode\n");
				break;
			}
			if (read_block(mem_diskptr, *ptr, temp_block) < 0) {
				printf("[sfs] @read_i reading from indirect block (*ptr = %d) ERROR\n", *ptr);
				return -1; //error in read
			}
			memcpy(data, temp_block + block_offset, bytes_toread);
			total_data_read += bytes_toread;
			data += bytes_toread;
			length -= bytes_toread;
			block_offset = 0;
		}
		FREE_BLOCK(indirect);
	}
	free(node);
	return total_data_read;
}


int* get_empty_blocks(int req_num_blocks) {
	int* block_ids = malloc(req_num_blocks * sizeof(int));
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @get_empty_blocks reading super_block ERROR\n");
		return NULL; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));
	/* Find the leftmost unset bit */
	int free_inode_pos = -1, idx = 0;
	for (int i = 0; i < sb.data_blocks && idx < req_num_blocks; i++) {
		if (!get_bitmap(mem_bitmap.bitmap_data, i)) {
			block_ids[idx++] = i + sb.data_block_idx;
			set_bitmap(mem_bitmap.bitmap_data, i);
			printf("[sfs] @get_empty_blocks found an empty block on %d\n", block_ids[idx - 1]);
		}
	}
	return block_ids;
}


int write_i(int inumber, char *data, int length, int offset) {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}

	if (!get_bitmap(mem_bitmap.bitmap_inode, inumber)) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Inode number is not set__\n\n");
		return -1;
	}
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[sfs] @read_i reading super_block ERROR\n");
		return -1; //error in read
	}
	super_block sb;
	memcpy(&sb, temp_block, sizeof(super_block));
	//validating inode number
	if (inumber < 0 || inumber >= sb.inodes || length < 0 || offset < 0 || offset > 1029 * BLOCKSIZE) {
		return -1;
	}
	//validating inode
	inode *node = retrieve_inode(&sb, inumber);
	if (node == NULL) {
		printf("[sfs] [ERROR] __Remove File failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}
	if (offset > node->size) {
		printf("[sfs] [ERROR] __Read_i failed__\n [ERROR] __Offset greater than size__\n\n");
		return -1;
	}
	if (node->valid == 0) {
		free(node);
		printf("[sfs] [ERROR] @write_i file doesn't contain requested amount of data\n");
		return -1;
	}
	int total_data_written = 0;
	length = min(length, 1029 * BLOCKSIZE - offset);
	printf("[sfs] @write_i length to read is %d\n", length);
	printf("Inode_size:%d, inode->valid:%d, inode->direct[0]:%d, inode->direct[1]:%d\n",
	       node->size, node->valid, node->direct[0], node->direct[1]);
	if (node->size - offset < length) { // requested length is greater than data present after offset
		// length = node->size - offset;
		/*Need to extend file size*/
		int req_num_blocks = ceil((double)(length - (node->size - offset)) / BLOCKSIZE);
		printf("[sfs] @write_i req_num_blocks : %d\n", req_num_blocks);
		int* block_ids = get_empty_blocks(req_num_blocks);
		if (block_ids == NULL) {
			printf("[sfs] @write_i No more space left in the disk\n");
			return -1;
		}
		int i = 0;
		// for (i = 0; i < req_num_blocks; i++) {
		// }
		if (floor((double)(node->size) / BLOCKSIZE) + 1 < 5) {
			int idx = floor((double)(node->size) / BLOCKSIZE) + 1;
			if (node->size == 0)idx--;
			for (; idx < 5 && i < req_num_blocks; i++) {
				node->direct[idx] = block_ids[i];
				printf("node->direct[idx=%d]=%d\n", idx, node->direct[idx]);
			}
		}
		if (i < req_num_blocks) {
			if (node->size <= 5 * BLOCKSIZE) { // this means that an indirect block has not been allocated
				node->indirect = *get_empty_blocks(1);
			}
			int offset_indirect = ceil((double)max(node->size - 5 * BLOCKSIZE, 0) / sizeof(uint32_t));
			// read_block(mem_diskptr, node->indirect, temp_block);
			if (read_block(mem_diskptr, node->indirect, temp_block) < 0) {
				printf("[sfs] @write_i reading indirect block blocknr = %d ERROR\n", node->indirect);
				return -1; //error in read
			}
			int* ptr = temp_block;
			for (; (void*)ptr < temp_block + BLOCKSIZE && i < req_num_blocks; i++) {
				*ptr = block_ids[i];
				ptr++;
			}

			if (write_block(mem_diskptr, node->indirect, temp_block) < 0) {
				printf("[sfs] @write_i writing to indirect block blocknr = %d ERROR\n", node->indirect);
				return -1; //error in read
			}
		}
		free(block_ids);
	}
	node->size = offset + length;

	/*we have extended the file size if required, now need to copy content to it*/
	int start_idx = (double)offset / BLOCKSIZE, block_offset = offset % BLOCKSIZE, bytes_towrite;
	if (offset < 5 * BLOCKSIZE) {/*if offeset lies in direct blocks*/
		for (; length && (start_idx < 5); start_idx++) {
			bytes_towrite = BLOCKSIZE - block_offset;
			if (length < bytes_towrite) {
				bytes_towrite = length;
			}
			if (read_block(mem_diskptr, node->direct[start_idx], temp_block) < 0) {
				printf("[sfs] @write_i reading from direct block (node->direct[%d] = %d) ERROR\n", start_idx, node->direct[start_idx]);
				return -1; //error in read
			}
			memcpy(temp_block + block_offset, data, bytes_towrite);
			if (write_block(mem_diskptr, node->direct[start_idx], temp_block) < 0) {
				printf("[sfs] @write_i reading from direct block blocknr = %d ERROR\n", node->direct[start_idx]);
				return -1; //error in read
			}
			total_data_written += bytes_towrite;
			data += bytes_towrite;
			length -= bytes_towrite;
			block_offset = 0;
		}
	}
	else {
		start_idx -= 5;
	}
	DECL_BLOCK(indirect);
	if (length > 0 && start_idx >= 0) {
		read_block(mem_diskptr, node->indirect, indirect);
		int* ptr = indirect + start_idx, bytes_towrite;
		for (; length && ((void*)ptr < indirect + (node->size - 5 * BLOCKSIZE)); ptr++) {
			bytes_towrite = BLOCKSIZE - block_offset;
			if (length < BLOCKSIZE) {
				bytes_towrite = length;
			}
			if (mem_bitmap.bitmap_data[*ptr] == 0) {
				printf("[sfs] GRAVE INCONSISTENCIES!!! some of the blocks should \
					have had their bit set according to the size of file inode\n");
				break;
			}
			if (read_block(mem_diskptr, *ptr, temp_block) < 0) {
				printf("[sfs] @write_i reading from indirect block (*ptr = %d) ERROR\n", *ptr);
				return -1; //error in read
			}
			memcpy(temp_block + block_offset, data, bytes_towrite);
			if (write_block(mem_diskptr, *ptr, temp_block) < 0) {
				printf("[sfs] @write_i writing to indirect block (*ptr = %d) ERROR\n", *ptr);
				return -1; //error in read
			}
			total_data_written += bytes_towrite;
			data += bytes_towrite;
			length -= bytes_towrite;
			block_offset = 0;
		}
		if (write_block(mem_diskptr, node->indirect, indirect) < 0) {
			printf("[sfs] @write_i writing back indirect block to disk (node->indirect = %d)ERROR\n", node->indirect);
		}
	}
	FREE_BLOCK(indirect);
	write_inode(&sb, inumber, node);
	free(node);
	return total_data_written;
}



int read_data_block(int entries_read, int num_files, inode *node, dir_block *dir_entries) {
	/* Read the list of files and subdirectories */
	for (int i = 0; i < 5; i++) {
		if (entries_read < num_files) {
			if (read_block(mem_diskptr, node->direct[i], temp_block) < 0) {
				printf("[ERROR] __Create Directory failed__\n [ERROR] __readblock failed__\n\n");
				return -1;
			}
			memcpy(dir_entries + entries_read, temp_block, min(BLOCKSIZE / sizeof(dir_block), num_files - entries_read));
			entries_read += min(BLOCKSIZE / sizeof(dir_block), num_files - entries_read);
		}
		else {
			break;
		}
	}
	if (entries_read < num_files) {
		int indirect_ptrs[1024];
		if (read_block(mem_diskptr, node->indirect, indirect_ptrs) < 0) {
			printf("[ERROR] __Create Directory failed__\n [ERROR] __readblock failed__\n\n");
			return -1;
		}
		for (int i = 0; i < BLOCKSIZE / sizeof(int); i++) {
			if (entries_read < num_files) {
				if (read_block(mem_diskptr, indirect_ptrs[i], temp_block) < 0) {
					printf("[ERROR] __Create Directory failed__\n [ERROR] __readblock failed__\n\n");
					return -1;
				}
				memcpy(dir_entries + entries_read, temp_block, min(BLOCKSIZE / sizeof(dir_block), num_files - entries_read));
				entries_read += min(BLOCKSIZE / sizeof(dir_block), num_files - entries_read);
			}
			else
				break;
		}
	}
	return 0;
}


int create_dir(char *dirpath) {
	if (mem_diskptr == NULL || STATE == UNMOUNTED) {
		printf("[ERROR] __Create Directory failed__\n [ERROR] __Disk unmounted__\n\n");
		return -1;
	}

	int ret;
	super_block sb;
	if (read_block(mem_diskptr, 0, temp_block) < 0) {
		printf("[ERROR] __Create Directory failed__\n [ERROR] __Super Block read failed__\n\n");
		return -1;
	}
	memcpy(&sb, temp_block, sizeof(super_block));

	/* Get the root directory data block
	   First Inode is reserved for root directory
	*/
	inode *node = retrieve_inode(&sb, 0);
	if (node == NULL) {
		printf("[ERROR] __Create Directory failed__\n [ERROR] __Unknown Error occured__\n\n");
		return -1;
	}

	/* Root directory not initialised */
	if (node->valid == 0) {
		if (strcmp(dirpath, "/") == 0) {
			node->valid = 1;
		}
		else {
			printf("[ERROR] __Create Directory failed__\n [ERROR] __Root not present__\n\n");
			return -1;
		}
	}

	/* Get the number of files */
	int num_files = node->size / sizeof(dir_block);
	dir_block *dir_entries = (dir_block *)malloc((num_files + 1) * sizeof(dir_block));
	int entries_read = 0;

	if (read_data_block(entries_read, num_files, node, dir_entries) < 0)
		return -1;

	char *token;
	token = strtok(dirpath, "/");
	if (token == NULL) {
		printf("[ERROR] __Create Directory failed__\n [ERROR] __Unknown error occurred__\n\n");
		return -1;
	}

	char* dirname;
	dirname = token;
	int parent_inode, inumber = 0;
	while (token != NULL) {
		int file_found = 0;

		/* Search for folder or files */
		for (int i = 0; i < num_files; i++) {

			if (dir_entries[i].valid && (dir_entries[i].file_name, token) == 0) {
				parent_inode = inumber;
				inumber = dir_entries[i].inumber;

				if (dir_entries[i].type == 0) {
					printf("[ERROR] __Create Directory failed__\n [ERROR] __Unknown error occurred__\n\n");
					return -1;
				}

				inode *node = retrieve_inode(&sb, 0);
				if (node == NULL) {
					printf("[ERROR] __Create Directory failed__\n [ERROR] __Unknown Error occured__\n\n");
					return -1;
				}

				num_files = node->size / sizeof(dir_block);
				dir_block *dir_entries = (dir_block *)malloc(sizeof(dir_block) * (num_files + 1));
				int entries_read = 0;

				if (read_data_block(entries_read, num_files, node, dir_entries) < 0)
					return -1;

				file_found = 1;
				break;
			}
		}

		if (!file_found) {
			dirname = token;
			token = strtok(NULL, "/");

			// If not return error
			if (token != NULL) {
				return -1;
			}

			// Create new file for the directory. Search for a empty location in the parent's list
			int new_dir_inode = create_file();
			int dir_entry_i;
			for (dir_entry_i = 0; dir_entry_i < num_files; dir_entry_i++) {
				if (!dir_entries[dir_entry_i].valid) {
					break;
				}
			}
			if (dir_entry_i == num_files)
				num_files++;

			// Initialise the directory entry as valid, with given folder name
			dir_entries[dir_entry_i].valid = 1;
			dir_entries[dir_entry_i].inumber = new_dir_inode;
			dir_entries[dir_entry_i].type = 1;
			strcpy(dir_entries[dir_entry_i].file_name, dirname);
			dir_entries[dir_entry_i].length = min(100, strlen(dirname));

			int entries_written = 0;
			int direct_i = 0;

			if (read_data_block(entries_read, num_files, node, dir_entries) < 0)
				return -1;

		}
		dirname = token;
		token = strtok(NULL, "/");
	}

	return 0;
}