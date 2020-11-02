#include "disk.h"
#include "sfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(){
	disk diskptr;
	int ret;

	ret = create_disk(&diskptr, 40984);
	printf("return value: %d\n\n", ret);

	char w_data[4096] = "This is a test";
	w_data[14] = '\0';



	ret = write_block(&diskptr, 0, (void *)w_data);
	printf("return value: %d\n\n", ret);

	void *r_data = (void *)malloc(BLOCKSIZE);
	ret = read_block(&diskptr, 0, r_data);
	printf("Here: %s\n", (char *)r_data);
	printf("return value: %d\n\n", ret);


	ret = free_disk(&diskptr);
	printf("return value: %d\n\n", ret);

	return 0;
}