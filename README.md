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