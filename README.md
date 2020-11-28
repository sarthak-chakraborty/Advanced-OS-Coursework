# Advances in Operating System Design (Coursework)

Assignments done by Sarthak Chakraborty and Sankalp Ramesh

### Assignment 1
- **Part A** - Building and Installing the Linux Kernel. Changes were made in the menucofig files of the linux kernel 5.2.6 and installed on the system.  
- **Part B** - Implement a min and max heap as a Loadable Kernel Module. Concurrency Control should be maintained and each process opening the `procfile` should have its own separate heap.  
    - **A1** - Use `read()` and `write()` system calls to initialize heap (min heap or max heap based on a flag) and its size, insert numbers and extract the top element of the heap.  
    - **A2** - Use `ioctl()` calls for initializing heap, insert numbers into the heap, extract information, and pop the top node. All the functions had different commands through which separate actions can be performed.

To run the kernel modules, follow the following instructions:
        
    * sudo su
    * make
    * insmod lkm_module<A1/A2>.ko
    * gcc userspace.c -o user
    * ./user
    * rmmod lkm_module<A1/A2>


### Assignment 2
- **Part A** - *Building an In-Memory Disk Emulator*: Write the functions corresponding to `create_disk()`, `read_block()`, `write_block()` and `free_disk()`. Read 4kB of data from the block and write 4kB of into the block.
- **Part B** - *Simple File System (SFS)*: Build a file system on top of the emulated disk. Maintain the super block, inodes and inode block, inode bitmap, data blocks, data bitmap and indirect blocks.
- **Part C** - *File and Directory Structure in SFS*: Create a directory structure in SFS and maintain all the files in it. **Note: This part is incomplete**

To run the SFS, go into the directory `Assignment2/CodeSubmission/` and then:

    * make
    * ./main