/*Assignment 1
------------------------------------------
Sankalp R. 16CS30031
Sarthak Charkraborty 16CS30044
------------------------------------------
*/
#ifndef DISK_H
#define DISK_H
#include<stdint.h>
#include <string.h>
const static int BLOCKSIZE = 4 * 1024;
#define DECL_BLOCK(b) void* (b) = malloc(BLOCKSIZE)
#define FREE_BLOCK(b) free((b))
typedef struct disk {
	uint32_t size; // size of the disk
	uint32_t blocks; // number of usable blocks (except stat block)
	uint32_t reads; // number of block reads performed
	uint32_t writes; // number of block writes performed
	char **block_arr; // array (of size blocks) of pointers to 4KB blocks
} disk;


int create_disk(disk *diskptr, int nbytes);

int read_block(disk *diskptr, int blocknr, void *block_data);

int write_block(disk *diskptr, int blocknr, void *block_data);

int free_disk(disk *diskptr);
#endif