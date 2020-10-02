#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h> // > for kmalloc_array
#define DEVICE_NAME "partb_1_16CS30031"


MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality

// static struct pb2_set_type_arguments {
// 	int32_t heap_size; // size of the heap
// 	int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
// } pb2_set_type_arguments;

// static struct obj_info {
// 	int32_t heap_size; // size of the heap
// 	int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
// 	int32_t root; // value of the root node of the heap (null if size is 0).
// 	int32_t last_inserted; // value of the last element inserted in the heap.
// } obj_info;

// static struct result {
// 	int32_t result; // value of min/max element extracted.
// 	int32_t heap_size; // size of the heap after extracting.
// } result;

static struct Heap {
	int32_t *arr;
	int32_t count;
	int32_t capacity;
	int32_t heap_type; // 0 for min heap , 1 for max heap
	int32_t last_inserted;
};
typedef struct Heap Heap;

/* Function Prototypes */
/* Heap methods
*/
// static long initialize_user_heap();
// static long insert_into_user_heap();
// static long pop_from_user_heap();
static Heap *CreateHeap(int32_t capacity, int32_t heap_type);
static int32_t DestroyHeap(Heap* heap);
static void insert(Heap *h, int32_t key);
// static void print(Heap *h);
static void heapify_bottom_top(Heap *h, int32_t index);
static void heapify_top_bottom(Heap *h, int32_t parent_node);
static int32_t PopMin(Heap *h);

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static char buffer[256] = {0};
static int buffer_len = 0;
static int num;
static int args_set = 0;
static int retval = -1;
static struct pb2_set_type_arguments pb2_args;
static struct obj_info heap;
static Heap* global_heap;

// static struct file_operations file_ops;
static struct file_operations file_ops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl,
};

static Heap *CreateHeap(int32_t capacity, int32_t heap_type) {
	// using void * kmalloc_array(size_t n, size_t size, gfp_t flags)
	Heap *h = (Heap * ) kmalloc(sizeof(Heap), GFP_ATOMIC); //one is number of heap

	//check if memory allocation is fails
	if (h == NULL) {
		printk(KERN_ALERT "Memory Error!");
		return NULL;
	}
	h->heap_type = heap_type;
	h->count = 0;
	h->capacity = capacity;
	h->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_ATOMIC); //size in bytes

	//check if allocation succeed
	if ( h->arr == NULL) {
		printk(KERN_ALERT "Memory Error!");
		return NULL;
	}
	return h;
}

static int32_t DestroyHeap(Heap* heap) {
	if (heap == NULL)
		return -1; // heap is not allocated
	kfree_const(heap->arr); // Function calls kfree only if x is not in .rodata section.
	kfree_const(heap);
	return 0;
}

static void insert(Heap *h, int32_t key) {
	if ( h->count < h->capacity) {
		h->arr[h->count] = key;
		heapify_bottom_top(h, h->count);
		h->count++;
		h->last_inserted = key;
	}
}

static void heapify_bottom_top(Heap *h, int32_t index) {
	int32_t temp;
	int32_t parent_node = (index - 1) / 2;

	if (h->arr[parent_node] > h->arr[index]) {
		//swap and recursive call
		temp = h->arr[parent_node];
		h->arr[parent_node] = h->arr[index];
		h->arr[index] = temp;
		heapify_bottom_top(h, parent_node);
	}
}

static void heapify_top_bottom(Heap *h, int32_t parent_node) {
	int32_t left = parent_node * 2 + 1;
	int32_t right = parent_node * 2 + 2;
	int32_t min;
	int32_t temp;

	if (left >= h->count || left < 0)
		left = -1;
	if (right >= h->count || right < 0)
		right = -1;

	if (left != -1 && h->arr[left] < h->arr[parent_node])
		min = left;
	else
		min = parent_node;
	if (right != -1 && h->arr[right] < h->arr[min])
		min = right;

	if (min != parent_node) {
		temp = h->arr[min];
		h->arr[min] = h->arr[parent_node];
		h->arr[parent_node] = temp;

		// recursive  call
		heapify_top_bottom(h, min);
	}
}
static int32_t top(Heap *h) {
	int32_t top;
	if (h->count == 0) {
		printk(KERN_INFO "\n__Heap is Empty__\n");
		return -EACCES;
	}
	// replace first node by last and delete last
	top = h->arr[0];
	return top;
}
static int32_t PopMin(Heap *h) {
	int32_t pop;
	if (h->count == 0) {
		// printk(KERN_INFO "\n__Heap is Empty__\n");
		return -1;
	}
	// replace first node by last and delete last
	pop = h->arr[0];
	h->arr[0] = h->arr[h->count - 1];
	h->count--;
	heapify_top_bottom(h, 0);
	return pop;
}
// static void print(Heap *h) {
// 	int32_t i;
// 	printk(KERN_INFO "____________Print Heap_____________\n");
// 	for (i = 0; i < h->count; i++) {
// 		printk(KERN_INFO "-> %d ", h->arr[i]);
// 	}
// 	printk(KERN_INFO "->__/\\__\n");
// }
static ssize_t dev_write(struct file *file, const char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;

	if (copy_from_user(buffer, buf, count < 256 ? count : 256))
		return -ENOBUFS;

	buffer_len = count < 256 ? count : 256;

	printk(KERN_INFO "%.*s", (int)count, buf);
	return buffer_len;
}


static ssize_t dev_read(struct file *file, char* buf, size_t count, loff_t* pos) {
	int ret = buffer_len;

	if (!buffer_len)
		return 0;
	if (!buf || !count)
		return -EINVAL;

	if (copy_to_user(buffer, buf, buffer_len))
		return -ENOBUFS;

	printk(KERN_INFO "%.*s", (int)buffer_len, buffer);
	buffer_len = 0;

	return ret;
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep) {

	// if (!mutex_trylock(&ebbchar_mutex)) {                // Try to acquire the mutex (returns 0 on fail)
	// 	printk(KERN_ALERT "EBBChar: Device in use by another process");
	// 	return -EBUSY;
	// }
	numberOpens++;
	printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
	return 0;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
	// mutex_unlock(&ebbchar_mutex);                      // release the mutex (i.e., lock goes up)
	printk(KERN_INFO "EBBChar: Device successfully closed\n");
	// DestroyHeap(global_heap);
	return 0;
}


static int hello_init(void) {
	struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0, NULL, &file_ops);
	if (!entry)
		return -ENOENT;

	// file_ops.owner = THIS_MODULE;
	// file_ops.dev_write = write;
	// file_ops.dev_read = read;
	// file_ops.unlocked_ioctl = dev_ioctl;

	printk(KERN_ALERT "Hello world\n");
	return 0;
}

static void hello_exit(void) {
	remove_proc_entry(DEVICE_NAME, NULL);

	printk(KERN_ALERT "Goodbye\n");
}

// static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
// 	switch (cmd) {
// 	case PB2_SET_TYPE:
// 		;
// 		retval = copy_from_user(&pb2_args, (struct pb2_set_type_arguments *)arg, sizeof(pb2_args));
// 		if (retval)
// 			return -1;

// 		printk("HEAP TYPE: %d", pb2_args.heap_type);
// 		printk("HEAP SIZE: %d", pb2_args.heap_size);


// 		if (pb2_args.heap_type != 0 && pb2_args.heap_type != 1)
// 			return -EINVAL;

// 		DestroyHeap(global_heap); // destroy any existing heap before creating a new one
// 		global_heap = CreateHeap(pb2_args.heap_size, pb2_args.heap_type); // allocating space for new heap


// 		args_set = 1;
// 		break;

// 	case PB2_INSERT:
// 		if (args_set == 0) {
// 			printk(KERN_ALERT "argset=0\n");
// 			return -EACCES;
// 		}

// 		// If heap is full, return EACCESS

// 		retval = copy_from_user(&num, (int *)arg, sizeof(int));

// 		printk("num: %d\n", num);
// 		insert(global_heap, num);

// 		break;
// 	case PB2_GET_INFO:
// 		if (args_set == 0)
// 			return -EACCES;

// 		heap.heap_type = pb2_args.heap_type;
// 		heap.heap_size = global_heap->count;
// 		heap.root = top(global_heap);
// 		heap.last_inserted = global_heap->last_inserted;

// 		retval = copy_to_user((struct obj_info *)arg, &heap, sizeof(struct obj_info));

// 		if (retval)
// 			return -1;

// 		break;

// 	case PB2_EXTRACT:
// 		if (args_set == 0)
// 			return -EACCES;
// 		struct result res =
// 		{
// 			.result = PopMin(global_heap),
// 			.heap_size = global_heap->count,
// 		};
// 		retval = copy_to_user((struct result *)arg, &res, sizeof(struct result));
// 		if (retval)
// 			return -1;
// 		break;

// 	default:
// 		return -EINVAL;

// 	}
// 	return 0;
// }
module_init(hello_init);
module_exit(hello_exit);

