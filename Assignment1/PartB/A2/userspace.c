#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

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

    int pid = fork();
    fork(); fork();
    /*
    int32_t tmp;
    int32_t val[10] = {9, 3, 6, 4, 2, 5, 2, 12, 15, 7};
    int32_t minsorted[10] = {2, 2, 3, 4, 5, 6, 7, 9, 12, 15};
    int32_t maxsorted[10] = {15, 12, 9, 7, 6, 5, 4, 3, 2, 2};
    */
    
    int32_t tmp;
    int32_t val[10];
    int32_t minsorted[10];
    int32_t maxsorted[10];

    if (pid == 0) {
        val[0] = 9;
        val[1] = 3;
        val[2] = 6;
        val[3] = 4;
        val[4] = 2;

        minsorted[0] = 2;
        minsorted[1] = 3;
        minsorted[2] = 4;
        minsorted[3] = 6;
        minsorted[4] = 9;
    }
    else{
        val[0] = 19;
        val[1] = 13;
        val[2] = 16;
        val[3] = 14;
        val[4] = 12;

        minsorted[0] = 12;
        minsorted[1] = 13;
        minsorted[2] = 14;
        minsorted[3] = 16;
        minsorted[4] = 19;
    }
    

	int fd, value, number;
	fd = open("/proc/partb_2_16CS30044", O_RDWR);
	if (fd < 0){
		perror("Error: ");
		return -1;
	}

	printf("File opened\n");

	// int ret;
	// struct pb2_set_type_arguments pb2_args;
	
	// Initialize min heap =============================================
    	printf("==== Initializing Min Heap ====\n");
	struct pb2_set_type_arguments pb2_args;
	// min heap
	pb2_args.heap_type = 0;
	// size of the heap
	pb2_args.heap_size = 10;

	int ret = ioctl(fd, PB2_SET_TYPE, &pb2_args);
	if(ret < 0){
        	perror("Initialization failed");
        	close(fd);
	        return 0;
   	}

	
	struct obj_info myobj_info;
	// Check info
	ret = ioctl(fd, PB2_GET_INFO, &myobj_info);
    	if(myobj_info.heap_size == 0){
        	printf(GRN "Size Matched\n" RESET);
    	}
    	else {
        	printf(RED "ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_size);
    	}

    	if(myobj_info.heap_type == 0){
        	printf(GRN "Type Matched\n" RESET);
    	}
    	else {
        	printf(RED "ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_type);
    	}

    	if(myobj_info.root == NULL){
        	printf(GRN "ROOT Matched\n" RESET);
    	}
    	else {
        	printf(RED "ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)NULL, (int)myobj_info.root);
    	}

    	if(myobj_info.last_inserted == NULL){
        	printf(GRN "last_inserted Matched\n" RESET);
    	}
    	else {
        	printf(RED "ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)NULL, (int)myobj_info.last_inserted);
    	}
	
	/*
	int32_t tmp;
	int32_t val[10] = {9, 3, 6, 4, 2, 5, 2, 12, 15, 7};
	int32_t minsorted[10] = {2, 2, 3, 4, 5, 6, 7, 9, 12, 15};
	int32_t maxsorted[10] = {15, 12, 9, 7, 6, 5, 4, 3, 2, 2};
    */

	struct obj_info heap_info;
	struct result res;
	int num;

	// =============================================
    /*
	char c;
	printf("\nPRESS ENTER to continue...\n");
	scanf("%c",&c);
    */
	// =============================================
	//
	
	// Test Min Heap ====================================================
	printf("======= Test Min Heap =========\n");

    	// Insert --------------------------------------------------------
	for (int i = 0; i < 5; i++)
    	{
        	printf("Inserting %d\n", val[i]);
        	ret = ioctl(fd, PB2_INSERT, &val[i]);
        	if(ret < 0){
            		perror("ERROR! Write failed");
            		close(fd);
            		return 0;
        	}
        }

	ret = ioctl(fd, PB2_GET_INFO, (struct obj_info *) &heap_info);
        if(ret == 0){
                   printf("\n================HEAP INFO=================\n");
        	   printf("Heap Type: %d\n", heap_info.heap_type);
                   printf("Heap Size: %d\n", heap_info.heap_size);
               	   printf("Lst inserted: %d\n", heap_info.last_inserted);
        }
	printf("\n");

	// Verify ---------------------------------------------------------
    	for (int i = 0; i < 5; i++)
    	{
        	printf("Extracting..\n");
        	ret = ioctl(fd, PB2_EXTRACT, (struct result *) &res);
        	if(ret < 0){
            		perror(RED "ERROR! Read failed\n" RESET);
            		close(fd);
            		return 0;
        	}
        	printf("Extracted: %d\n", res.result);
        	if(res.result == minsorted[i]){
            		printf(GRN "Results Matched\n" RESET);
        	}
        	else {
            		printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)minsorted[i], res.result);
        	}
    	}
   	 close(fd);
       	
	/*	
	// Test Max Heap ====================================================
	fd = open("/proc/partb_2_16CS30044", O_RDWR);
        if (fd < 0){
                perror("Error: ");
                return -1;
        }

        printf("File opened\n");

        // Initialize max heap =============================================
        printf("==== Initializing Max Heap ====\n");
        // max heap
        pb2_args.heap_type = 1;
        // size of the heap
        pb2_args.heap_size = 10;

        ret = ioctl(fd, PB2_SET_TYPE, &pb2_args);
        if(ret < 0){
                perror("Initialization failed");
                close(fd);
                return 0;
        }

	// Test Max Heap ====================================================
        printf("======= Test Max Heap =========\n");

        // Insert --------------------------------------------------------
        for (int i = 0; i < 10; i++)
        {
                printf("Inserting %d\n", val[i]);
                ret = ioctl(fd, PB2_INSERT, &val[i]);
                if(ret < 0){
                        perror("ERROR! Write failed");
                        close(fd);
                        return 0;
                }
        }

        ret = ioctl(fd, PB2_GET_INFO, (struct obj_info *) &heap_info);
        if(ret == 0){
                   printf("\n===========HEAP INFO==============\n");
                   printf("Heap Type: %d\n", heap_info.heap_type);
                   printf("Heap Size: %d\n", heap_info.heap_size);
                   printf("Lst inserted: %d\n", heap_info.last_inserted);
        }
	printf("\n");

	// Verify ---------------------------------------------------------
        for (int i = 0; i < 10; i++)
        {
                printf("Extracting..\n");
                ret = ioctl(fd, PB2_EXTRACT, (struct result *) &res);
                if(ret < 0){
                        perror(RED "ERROR! Read failed\n" RESET);
                        close(fd);
                        return 0;
                }
                printf("Extracted: %d\n", res.result);
                if(res.result == maxsorted[i]){
                        printf(GRN "Results Matched\n" RESET);
                }
                else {
                        printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)maxsorted[i], res.result);
                }
        }
         close(fd);
	*/


	return 0;
}
