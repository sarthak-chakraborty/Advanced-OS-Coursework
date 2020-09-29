#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)


struct pb2_set_type_arguments
{
	int32_t heap_size;	// size of the heap
	int32_t heap_type;	// heap type: 0 for min-heap, 1 for max-heap
};



int main(){
	int fd, value, number;
	fd = open("/proc/partb_2_16CS30044", O_RDWR);
	if (fd < 0)
		return 1;

	struct pb2_set_type_arguments pb2_args;
	pb2_args.heap_type = 0;
	pb2_args.heap_size = 4;

	ioctl(fd, PB2_SET_TYPE, (struct pb2_set_type_arguments *) &pb2_args);
	printf("Value is %d\n", value);

	close(fd);

	return 0;
}
