#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define READ_DATA _IOR('a','b',int32_t*)


static struct file_operations file_ops;
static char buffer[256] = {0};
static int buffer_len = 0;


static long ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case READ_DATA:
			copy_to_user((int*) arg, &buffer_len, sizeof(buffer_len));
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
