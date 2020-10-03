#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

#define PB2_SET_TYPE    _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT      _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO    _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT     _IOR(0x10, 0x34, int32_t*)


struct pb2_set_type_arguments { 
    int32_t heap_size; // size of the heap
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
};
struct obj_info {
    int32_t heap_size; // current number of elements in heap (current size of the heap)
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
    int32_t root; // value of the root node of the heap (null if size is 0).
    int32_t last_inserted; // value of the last element inserted in the heap.
};

struct result {
    int32_t result;         // value of min/max element extracted.
    int32_t heap_size;     // current size of the heap after extracting.
};



int main(int argc, char *argv[]){
    if( argc != 2 ) {
        printf("Please provide your LKM proc file name as an argument.\nExample:\n./test_ass1 partb_1_<roll no>\n");
        return -1;
    }
    struct pb2_set_type_arguments mypb2_set_type_arguments;
    struct obj_info myobj_info;
    struct result myresult;


    char procfile[100] = "/proc/";
    strcat(procfile, argv[1]);

    int fd = open(procfile, O_RDWR) ;

    if(fd < 0){
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    
    // Initialize min heap =============================================
    printf("==== Initializing Min Heap ====\n");
    char buf[2];
    // min heap
    mypb2_set_type_arguments.heap_type = 0;
    // size of the heap
    mypb2_set_type_arguments.heap_size = 20;

    int result = ioctl(fd, PB2_SET_TYPE, &mypb2_set_type_arguments);

    if(result < 0){
        perror("Initialization failed");
        close(fd);
        return 0;
    }

    // Check info
    result = ioctl(fd, PB2_GET_INFO, &myobj_info);
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


    int32_t tmp;
    int32_t val[10] = {9, 3, 6, 4, 2, 5, 2, 12, 15, 7};
    int32_t minsorted[10] = {2, 2, 3, 4, 5, 6, 7, 9, 12, 15};
    int32_t maxsorted[10] = {15, 12, 9, 7, 6, 5, 4, 3, 2, 2};


    // Test Min Heap ====================================================
    printf("======= Test Min Heap =========\n");

    // Insert --------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Inserting %d\n", val[i]);
        result = ioctl(fd, PB2_INSERT, &val[i]);
        if(result < 0){
            perror("ERROR! Insert failed");
            close(fd);
            return 0;
        }
    }

    // Check info
    result = ioctl(fd, PB2_GET_INFO, &myobj_info);
    if(myobj_info.heap_size == 10){
        printf(GRN "Size Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, 10, (int)myobj_info.heap_size);
    }

    if(myobj_info.heap_type == 0){
        printf(GRN "Type Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, 0, (int)myobj_info.heap_type);
    }

    if(myobj_info.root == 2){
        printf(GRN "ROOT Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)2, (int)myobj_info.root);
    }

    if(myobj_info.last_inserted == 7){
        printf(GRN "last_inserted Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)7, (int)myobj_info.last_inserted);
    }
    
    // Verify ---------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Extracting..\n");
        result = ioctl(fd, PB2_EXTRACT, &myresult);
        if(result < 0){
            perror(RED "ERROR! Read failed\n" RESET);
            close(fd);
            return 0;
        }
        printf("Extracted: %d\n", (int)tmp);
        if(myresult.result == minsorted[i]){
            printf(GRN "Results Matched\n" RESET);
        }
        else {
            printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)minsorted[i], (int)tmp);
        }
    }

    close(fd);


    // // Test Max Heap ====================================================
    // fd = open(procfile, O_RDWR) ;

    // if(fd < 0){
    //     perror("Could not open flie /proc/ass1");
    //     return 0;
    // }
    // // Initialize min heap =============================================
    // printf("==== Initializing Max Heap ====\n");
    // // max heap
    // buf[0] = 0xF0;
    // // size of the heap
    // buf[1] = 20;

    // result = write(fd, buf, 2);
    // if(result < 0){
    //     perror("Write failed");
    //     close(fd);
    //     return 0;
    // }
    // printf("Written %d bytes\n", result);

    // printf("======= Test Max Heap =========\n");
    // // Insert --------------------------------------------------------
    // for (int i = 0; i < 10; i++)
    // {
    //     printf("Inserting %d\n", val[i]);
    //     result = write(fd, &val[i], sizeof(int32_t));
    //     if(result < 0){
    //         perror("ERROR! Write failed");
    //         close(fd);
    //         return 0;
    //     }
    //     printf("Written %d bytes\n", result);
    // }
    
    // // Verify ---------------------------------------------------------
    // for (int i = 0; i < 10; i++)
    // {
    //     printf("Extracting..\n");
    //     result = read(fd, (void *)(&tmp), sizeof(int32_t));
    //     if(result < 0){
    //         perror("ERROR! Read failed");
    //         close(fd);
    //         return 0;
    //     }
    //     printf("Extracted: %d\n", (int)tmp);
    //     if(tmp == maxsorted[i]){
    //         printf(GRN "Results Matched\n" RESET);
    //     }
    //     else {
    //         printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)maxsorted[i], (int)tmp);
    //     }
    // }

   return 0;
}