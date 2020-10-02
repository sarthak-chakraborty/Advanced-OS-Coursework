#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>


#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)


struct pb2_set_type_arguments{
	int32_t heap_size;	// size of the heap
	int32_t heap_type;	// heap type: 0 for min-heap, 1 for max-heap
};

struct obj_info{
	int32_t heap_size;	// size of the heap
	int32_t heap_type;	// heap type: 0 for min-heap, 1 for max-heap
	int32_t root;	// value of the root node of the heap (null if size is 0)
	int32_t last_inserted;	// value of the last element inserted in the heap
};

struct result{
	int32_t result;		// value of min/max element extracted
	int32_t heap_size;	// size of the heap after extracting
};


int main(){
	int fd, value, number;
	fd = open("/proc/partb_2_16CS30044", O_RDWR);
	if (fd < 0){
		perror("Error: ");
		return 1;
	}

	printf("File opened\n");

	struct pb2_set_type_arguments pb2_args;
	pb2_args.heap_type = 0;
	pb2_args.heap_size = 6;

	int ret = ioctl(fd, PB2_SET_TYPE, &pb2_args);
	printf("%d\n",ret);
	
	int num = 17;
	ret = ioctl(fd, PB2_INSERT, &num);
	printf("%d\n", ret);

	
	struct obj_info heap_info;
	//int n;

	ret = ioctl(fd, PB2_GET_INFO, (struct obj_info *) &heap_info);
	if(ret == 0){
		printf("HEAP INFO:\n");
		printf("%d\n",ret);
		printf("Heap Type: %d\n", heap_info.heap_type);
		printf("Heap Size: %d\n", heap_info.heap_size);
		//printf("%d\n",n);
	}
	

	close(fd);

	return 0;
}
