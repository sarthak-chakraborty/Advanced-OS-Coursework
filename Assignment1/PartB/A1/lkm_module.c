#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h> 
#include <linux/mutex.h>
#define DEVICE_NAME "partb_1_16CS30031"
#define INF  (0xffffffff)
const char MIN_HEAP = 0xFF, MAX_HEAP = 0xF0;

MODULE_LICENSE("GPL");


struct Heap {
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
static Heap *CreateHeap(int32_t capacity, char heap_type);
static Heap* DestroyHeap(Heap* heap);
static int32_t insert(Heap *h, int32_t key);
static void heapify_bottom_top(Heap *h, int32_t index);
static void heapify_top_bottom(Heap *h, int32_t parent_node);
static int32_t PopMin(Heap *h);

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


static int    numberOpens = 0;
static char buffer[256] = {0};
static int buffer_len = 0;
static int num;
static int args_set = 0;
static int retval = -1;

static Heap* global_heap;

static struct file_operations file_ops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static Heap *CreateHeap(int32_t capacity, char heap_type) {
	// using void * kmalloc_array(size_t n, size_t size, gfp_t flags)
	Heap *h = (Heap * ) kmalloc(sizeof(Heap), GFP_KERNEL); //one is number of heap

	//check if memory allocation is fails
	if (h == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": Memory Error!");
		return NULL;
	}
	h->heap_type = ((heap_type == MIN_HEAP) ? 0 : 1);
	h->count = 0;
	h->capacity = capacity;
	h->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); //size in bytes

	//check if allocation succeed
	if ( h->arr == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": Memory Error!");
		return NULL;
	}
	return h;
}

static Heap* DestroyHeap(Heap* heap) {
	if (heap == NULL)
		return -1; // heap is not allocated
	printk(KERN_INFO DEVICE_NAME ": %d bytes of heap->arr Space freed.\n", sizeof(heap->arr));
	kfree(heap->arr);
	kfree(heap);
	args_set = 0;
	return NULL;
}

static int32_t insert(Heap *h, int32_t key) {
	if ( h->count < h->capacity) {
		h->arr[h->count] = key;
		heapify_bottom_top(h, h->count);
		h->count++;
		h->last_inserted = key;
	}
	else {
		return -EACCES;
	}
	return 0;
}

static void heapify_bottom_top(Heap *h, int32_t index) {
	int32_t temp;
	int32_t parent_node = (index - 1) / 2;

	if(h->heap_type == 0){	// Min Heap
		if (h->arr[parent_node] > h->arr[index]) {
			//swap and recursive call
			temp = h->arr[parent_node];
			h->arr[parent_node] = h->arr[index];
			h->arr[index] = temp;
			heapify_bottom_top(h, parent_node);
		}
	}
	else{	// Max Heap
		if (h->arr[parent_node] < h->arr[index]) {
			//swap and recursive call
			temp = h->arr[parent_node];
			h->arr[parent_node] = h->arr[index];
			h->arr[index] = temp;
			heapify_bottom_top(h, parent_node);
		}
	}
}

static void heapify_top_bottom(Heap *h, int32_t parent_node) {
	int32_t left = parent_node * 2 + 1;
	int32_t right = parent_node * 2 + 2;
	int32_t temp;

	if (left >= h->count || left < 0)
		left = -1;
	if (right >= h->count || right < 0)
		right = -1;

	if(h->heap_type == 0){	// Min heap
		int32_t min;
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
	else{	// Max heap
		int32_t max;
		if (left != -1 && h->arr[left] > h->arr[parent_node])
			max = left;
		else
			max = parent_node;
		if (right != -1 && h->arr[right] > h->arr[max])
			max = right;

		if (max != parent_node) {
			temp = h->arr[max];
			h->arr[max] = h->arr[parent_node];
			h->arr[parent_node] = temp;

			// recursive  call
			heapify_top_bottom(h, max);
		}
	}
}
// static int32_t top(Heap *h) {
// 	int32_t top;
// 	if (h->count == 0) {
// 		printk(KERN_INFO DEVICE_NAME ": \n__Heap is Empty__\n");
// 		return -EACCES;
// 	}
// 	// replace first node by last and delete last
// 	top = h->arr[0];
// 	return top;
// }
static int32_t PopMin(Heap *h) {
	int32_t pop;
	if (h->count == 0) {
		// printk(KERN_INFO DEVICE_NAME ": \n__Heap is Empty__\n");
		return -INF;
	}
	// replace first node by last and delete last
	pop = h->arr[0];
	h->arr[0] = h->arr[h->count - 1];
	h->count--;
	heapify_top_bottom(h, 0);
	return pop;
}


static ssize_t dev_write(struct file *file, const char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;
	if (copy_from_user(buffer, buf, count < 256 ? count : 256))
		return -ENOBUFS;

	buffer_len = count < 256 ? count : 256;

	printk(KERN_INFO DEVICE_NAME ": %.*s", (int)count, buf);
	// printk(KERN_ALERT "VALUes :::: %d %d", buffer[0], buffer[1]);
	printk(KERN_ALERT DEVICE_NAME ": VALUes :::: %d %d", buf[0], buf[1]);
	// printk(KERN_ALERT "VALUes :::: %c %c", buffer[0], buffer[1]);

	if (buffer_len != 2 && buffer_len != 4) {
		printk(KERN_ALERT DEVICE_NAME ": WRONG DATA SENT. %d bytes", buffer_len);
		return -EINVAL;
	}
	if (args_set) {
		int32_t num ;
		memcpy(&num, buffer, sizeof(num));
		printk(DEVICE_NAME ": num: %d\n", num);
		int32_t ret;
		ret = insert(global_heap, num);
		if (ret < 0) // Heap is filled to capacity
			return -1;
		return sizeof(num);
	}


	// retval = copy_from_user(&pb2_args, (struct pb2_set_type_arguments *)arg, sizeof(pb2_args));
	// if (retval)
	// 	return -1;
	if (buffer_len != 2)
		return -EINVAL;
	char heap_type;
	int32_t heap_size;
	// buffer[7] = buffer[1];
	// buffer[3] = buffer[0];
	// buffer[0] = buffer[1] = buffer[2] = buffer[4] = buffer[5] = buffer[6] = 0;
	// memcpy(&heap_type, buffer, sizeof(char));
	// memcpy(&heap_size, buffer + 4, sizeof(char));
	heap_type = buf[0];
	heap_size = buf[1];
	printk(DEVICE_NAME ": HEAP TYPE: %d", heap_type);
	printk(DEVICE_NAME ": HEAP SIZE: %d", heap_size);
	printk(DEVICE_NAME ": RECIEVED:  %d bytes", count);



	if (heap_type != MIN_HEAP && heap_type != MAX_HEAP) {
		printk(KERN_ALERT DEVICE_NAME ": Wrong type!! %c\n", heap_type);
		return -EINVAL;
	}

	if (heap_size <= 0 || heap_size > 100) {
		printk(KERN_ALERT DEVICE_NAME ": Wrong size of heap %d!!\n", heap_size);
		return -EINVAL;
	}
	// global_heap = DestroyHeap(global_heap); // destroy any existing heap before creating a new one
	// global_heap = CreateHeap(pb2_args.heap_size, pb2_args.heap_type); // allocating space for new heap
	global_heap = DestroyHeap(global_heap); // destroy any existing heap before creating a new one
	global_heap = CreateHeap(heap_size, heap_type); // allocating space for new heap

	args_set = 1;
	return buffer_len;
}


static ssize_t dev_read(struct file *file, char* buf, size_t count, loff_t* pos) {
	// int ret = buffer_len;

	// if (!buffer_len)
	// 	return 0;
	if (!buf || !count)
		return -EINVAL;
	if (args_set == 0)
		return -EACCES;
	int32_t topnode;
	topnode = PopMin(global_heap);
	retval = copy_to_user(buf, (int32_t*)&topnode, sizeof(topnode));
	// if (retval || topnode == -INF)
	// 	return -1;
	if (retval == 0 && topnode != -INF) {    // success!
		printk(KERN_INFO DEVICE_NAME ": Sent %ld characters to the user\n", sizeof(topnode));
		return sizeof(topnode);
		// return (size_of_message = 0); // clear the position to the start and return 0
	}
	else {
		printk(KERN_INFO DEVICE_NAME ": Failed to send %d characters to the user\n", retval);
		return -1;      // Failed -- return a bad address message (i.e. -14)
	}
	// if (copy_to_user(buffer, buf, buffer_len))
	// 	return -ENOBUFS;

	// printk(KERN_INFO "%.*s", (int)buffer_len, buffer);
	// buffer_len = 0;

	// return sizeof(topnode);
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep) {

	// if (!mutex_trylock(&ebbchar_mutex)) {                // Try to acquire the mutex (returns 0 on fail)
	// 	printk(KERN_ALERT DEVICE_NAME ": Device in use by another process");
	// 	return -EBUSY;
	// }
	numberOpens++;
	printk(KERN_INFO DEVICE_NAME ": Device has been opened %d time(s)\n", numberOpens);
	return 0;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
	// mutex_unlock(&ebbchar_mutex);                      // release the mutex (i.e., lock goes up)
	printk(KERN_INFO DEVICE_NAME ": Device successfully closed\n");
	global_heap = DestroyHeap(global_heap);
	return 0;
}


static int hello_init(void) {
	struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0, NULL, &file_ops);
	if (!entry)
		return -ENOENT;

	global_heap = NULL;
	printk(KERN_ALERT DEVICE_NAME ": Hello world\n");
	return 0;
}

static void hello_exit(void) {
	global_heap = DestroyHeap(global_heap);
	remove_proc_entry(DEVICE_NAME, NULL);

	printk(KERN_ALERT DEVICE_NAME ": Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);

