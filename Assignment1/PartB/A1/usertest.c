#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#define DEVICE_NAME "partb_1_16CS30031"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

int main(int argc, char *argv[]) {
    // if ( argc != 2 ) {
    //     printf("Please provide your LKM proc file name as an argument.\nExample:\n./test_ass1 partb_1_<roll no>\n");
    //     return -1;
    // }
    char procfile[100] = "/proc/";
    // strcat(procfile, argv[1]);
    strcat(procfile, DEVICE_NAME);

    int pid = fork();

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

    int fd = open(procfile, O_RDWR) ;

    if (fd < 0) {
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    // Initialize min heap =============================================
    printf("==== Initializing Min Heap ====\n");
    char buf[2];
    // min heap
    buf[0] = 0xFF;
    // size of the heap
    buf[1] = 20;

    int result;
    result = write(fd, buf, 2);
    if (result < 0) {
        perror("Write failed");
        close(fd);
        return 0;
    }
    printf("Written %d bytes\n", result);



    // Test Min Heap ====================================================
    printf("======= Test Min Heap =========\n");

    // // Insert --------------------------------------------------------
    for (int i = 0; i < 5; i++)
    {
        printf("Inserting %d\n", val[i]);
        result = write(fd, &val[i], sizeof(int32_t));
        if (result < 0) {
            perror("ERROR! Write failed");
            close(fd);
            return 0;
        }
        printf("Written %d bytes\n", result);
    }
    // int fd1 = open(procfile, O_RDWR) ;
    //printf("Received this when tried to open it twice %d\n", fd1);
    // Verify ---------------------------------------------------------
    for (int i = 0; i < 5; i++)
    {
        printf("Extracting..\n");
        result = read(fd, (void *)(&tmp), sizeof(int32_t));
        if (result < 0) {
            perror(RED "ERROR! Read failed\n" RESET);
            close(fd);
            return 0;
        }
        printf("Extracted: %d\n", (int)tmp);
        if (tmp == minsorted[i]) {
            printf(GRN "Results Matched\n" RESET);
        }
        else {
            printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)minsorted[i], (int)tmp);
        }
    }

    close(fd);


    /*
    // Test Max Heap ====================================================
    fd = open(procfile, O_RDWR) ;

    if (fd < 0) {
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    // Initialize min heap =============================================
    printf("==== Initializing Max Heap ====\n");
    // max heap
    buf[0] = 0xF0;
    // size of the heap
    buf[1] = 20;

    result = write(fd, buf, 2);
    if (result < 0) {
        perror("Write failed");
        close(fd);
        return 0;
    }
    printf("Written %d bytes\n", result);

    printf("======= Test Max Heap =========\n");
    // Insert --------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Inserting %d\n", val[i]);
        result = write(fd, &val[i], sizeof(int32_t));
        if (result < 0) {
            perror("ERROR! Write failed");
            close(fd);
            return 0;
        }
        printf("Written %d bytes\n", result);
    }

    // Verify ---------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Extracting..\n");
        result = read(fd, (void *)(&tmp), sizeof(int32_t));
        if (result < 0) {
            perror("ERROR! Read failed");
            close(fd);
            return 0;
        }
        printf("Extracted: %d\n", (int)tmp);
        if (tmp == maxsorted[i]) {
            printf(GRN "Results Matched\n" RESET);
        }
        else {
            printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)maxsorted[i], (int)tmp);
        }
    }
    close(fd);
    */

    return 0;
}