#include "disk.h"
#include <stdio.h>
#include <stdlib.h>

const static int STAT_BLOCK_SIZE = 24;


int create_disk(disk *diskptr, int nbytes){
	printf("nbytes: %d\n", nbytes);
	int n_blocks = (int)((nbytes - STAT_BLOCK_SIZE) / BLOCKSIZE);

	printf("nblock: %d\n", n_blocks);

	diskptr->size = nbytes;
	diskptr->blocks = n_blocks;
	diskptr->reads = 0;
	diskptr->writes = 0;
	diskptr->block_arr = (char **)malloc(n_blocks * sizeof(char *));
	if(diskptr->block_arr == NULL)
		return -1;

	for(int i = 0; i < n_blocks; i++){
		diskptr->block_arr[i] = (char *)malloc(BLOCKSIZE);
		if(diskptr->block_arr[i] == NULL)
			return -1;
	}

	printf("DISK CREATED\n");

	return 0;
}


int read_block(disk *diskptr, int blocknr, void *block_data){
	int n_blocks = diskptr->blocks;

	if(blocknr < 0 || blocknr > n_blocks-1)
		return -1;

	memcpy(block_data, diskptr->block_arr[blocknr], BLOCKSIZE);
	diskptr->reads += 1;
	
	return 0;
}


int write_block(disk *diskptr, int blocknr, void *block_data){
	int n_blocks = diskptr->blocks;

	if(blocknr < 0 || blocknr > n_blocks-1)
		return -1;

	memcpy(diskptr->block_arr[blocknr], (char *)block_data, BLOCKSIZE);
	diskptr->writes += 1;
	
	return 0;
}


int free_disk(disk *diskptr){
	int n_blocks = diskptr->blocks;

	for(int i = 0; i < n_blocks; i++){
		free(diskptr->block_arr[i]);
	}
	free(diskptr->block_arr);

	printf("DISK FREED\n");

	return 0;
}