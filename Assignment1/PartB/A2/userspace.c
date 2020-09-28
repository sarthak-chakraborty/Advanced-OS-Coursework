#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define READ_DATA _IOR('a','b',int32_t*)


int main(){
	int fd, value, number;
	fd = open("/proc/partb_2_16CS30044", O_RDWR);
	if (fd < 0)
		return 1;

	ioctl(fd, READ_DATA, (int *) &value);
	printf("Value is %d\n", value);

	close(fd);

	return 0;
}
