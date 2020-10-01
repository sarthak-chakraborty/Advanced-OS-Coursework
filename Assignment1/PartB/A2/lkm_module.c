#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)


static struct pb2_set_type_arguments{
	int32_t heap_size; // size of the heap
	int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
}pb2_set_type_arguments;

static struct obj_info {
	int32_t heap_size; // size of the heap
	int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
	int32_t root; // value of the root node of the heap (null if size is 0).
	int32_t last_inserted; // value of the last element inserted in the heap.
}obj_info;


static struct result {
	int32_t result; // value of min/max element extracted.
	int32_t heap_size; // size of the heap after extracting.
}result;

static struct file_operations file_ops;
static char buffer[256] = {0};
static int buffer_len = 0;
static int num;

static int args_set = 0;

static struct pb2_set_type_arguments pb2_args;
static struct obj_info heap;


static long ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case PB2_SET_TYPE:
			;
			copy_from_user(&pb2_args, (struct pb2_set_type_arguments *)arg, sizeof(pb2_args));

			printk("HEAP TYPE: %d", pb2_args.heap_type);
			printk("HEAP SIZE: %d", pb2_args.heap_size);
			
			printk("num: %d", num);

			if (pb2_args.heap_type != 0 || pb2_args.heap_type != 1)
				return -EINVAL;

			args_set = 1;
			return 0;
			break;

		case PB2_INSERT:
			if (args_set == 0)
				return -EACCES;
			
			// If heap is full, return EACCESS

			int retval = copy_from_user(&num, &arg, sizeof(int));
			
			if(!retval){
				return -EINVAL;
			}

			return 0;
			break;

		case PB2_GET_INFO:
			if (args_set == 0)
				return -EACCES;
			
			int retval = copy_to_user(heap, sizeof(obj_info));
			break;

		case PB2_EXTRACT:
			if (args_set == 0)
				return -EACCES;

			break;

		default:
			return -EINVAL;
	}
	return 0;
}


static ssize_t write(struct file *file, const char* buf, size_t count, loff_t* pos){
	if(!buf || !count)
		return -EINVAL;

	if(copy_from_user(buffer, buf, count<256?count:256))
		return -ENOBUFS;

	buffer_len = count < 256 ? count : 256;

	printk(KERN_INFO "%.*s",(size_t)count, buf);
	return buffer_len;
}


static ssize_t read(struct file *file, char* buf, size_t count, loff_t* pos){
	int ret = buffer_len;

	if(!buffer_len)
		return 0;
	if(!buf || !count)
		return -EINVAL;

	if(copy_to_user(buffer, buf, buffer_len))
		return -ENOBUFS;

	printk(KERN_INFO "%.*s", (size_t)buffer_len, buffer);
	buffer_len = 0;

	return ret;
}



static int hello_init(void){
	struct proc_dir_entry *entry = proc_create("partb_2_16CS30044", 0, NULL, &file_ops);
	if(!entry)
		return -ENOENT;

	file_ops.owner = THIS_MODULE;
	file_ops.write = write;
	file_ops.read = read;
	file_ops.unlocked_ioctl = ioctl;

	printk(KERN_ALERT "Hello world\n");
	return 0;
}

static void hello_exit(void){
	remove_proc_entry("partb_2_16CS30044", NULL);

	printk(KERN_ALERT "Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);
