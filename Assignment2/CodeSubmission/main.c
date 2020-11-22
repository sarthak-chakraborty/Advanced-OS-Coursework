#include "disk.h"
#include "sfs.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	disk diskptr;
	int ret;

	ret = create_disk(&diskptr, 100 * 1024 * 4 + 24);
	printf("return value: %d\n\n", ret);

	char w_data[4096] = "This is a test";
	w_data[14] = '\0';



	ret = write_block(&diskptr, 0, (void *)w_data);
	printf("return value: %d\n\n", ret);

	void *r_data = (void *)malloc(BLOCKSIZE);
	ret = read_block(&diskptr, 0, r_data);
	printf("Here: %s\n", (char *)r_data);
	printf("return value: %d\n\n", ret);


	printf("Calling format...\n");
	ret = format(&diskptr);
	printf(">>>format return value: %d\n\n", ret);

	printf("Calling mount...\n");
	ret = mount(&diskptr);
	printf(">>>mount return value: %d\n\n", ret);

	printf("Calling create_file\n");
	int file1 = create_file();
	printf(">>>create_file return value: %d\n\n", file1);

	char data[5] = "abcd";
	int offset = 0;
	printf("Calling write_i\n");
	ret = write_i(file1, data, 4, offset);
	printf(">>>write_i return value: %d\n\n", ret);

	char buf[10] = "xxxxxxx";
	int toread = 1;
	printf("Calling read_i\n");
	offset = 2;
	ret = read_i(file1, buf, toread, offset);
	printf("READ this into buf :\n");
	for (int i = 0; i < toread; i++) {
		printf("%c\n", buf[i]);
	}
	printf(">>>read_i return value: %d\n\n", ret);

	printf("Calling write_i\n");
	ret = write_i(file1, data, 4, offset = 3);
	printf(">>>write_i return value: %d\n\n", ret);

	printf("Calling read_i\n");
	toread = 7;
	offset = 0;
	ret = read_i(file1, buf, toread, offset);
	printf("READ this into buf :\n");
	for (int i = 0; i < toread; i++) {
		printf("%c\n", buf[i]);
	}
	printf(">>>read_i return value: %d\n\n", ret);
	printf("Calling remove_file\n");
	ret = remove_file(file1);
	printf(">>>remove_file return value: %d\n\n", ret);

	printf("Calling free_disk\n");
	ret = free_disk(&diskptr);
	printf(">>>free_disk return value: %d\n\n", ret);
	return 0;
}