/*
PART B: Assignment 2
------------------------------------------
Sankalp R. 16CS30031
Sarthak Charkraborty 16CS30044
------------------------------------------
Kernel Version used : 5.4.0-48-generic
System : Ubuntu 18.04 LTS
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h> 
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>

#define DEVICE_NAME "partb_2_16CS30044"
#define INF  (0xffffffff)
#define current get_current()
const char MIN_HEAP = 0xFF, MAX_HEAP = 0xF0;

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)

MODULE_LICENSE("GPL");
static DEFINE_MUTEX(heap_mutex);

struct pb2_set_type_arguments {
	int32_t heap_size; // size of the heap
	int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
};

struct obj_info {
	int32_t heap_size; // size of the heap
	int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
	int32_t root; // value of the root node of the heap (null if size is 0).
	int32_t last_inserted; // value of the last element inserted in the heap.
};

struct result {
	int32_t result; // value of min/max element extracted.
	int32_t heap_size; // size of the heap after extracting.
};

struct Heap {
	int32_t *arr;
	int32_t count;
	int32_t capacity;
	int32_t heap_type; // 0 for min heap , 1 for max heap
	int32_t last_inserted;
};
typedef struct Heap Heap;

struct h_struct {
	int key;
	Heap* global_heap;
	struct h_struct* next;
};
struct h_struct *htable, *entry;


/* Function Prototypes */
/* Heap methods */
static Heap* CreateHeap(int32_t capacity, int32_t heap_type);
static Heap* DestroyHeap(Heap* heap);
static int32_t insert(Heap *h, int32_t key);
static void heapify_bottom_top(Heap *h, int32_t index);
static void heapify_top_bottom(Heap *h, int32_t parent_node);
static int32_t PopMin(Heap *h);

/* Concurrency Control Methods */
static struct h_struct* get_entry_from_key(int key);
static void key_add(struct h_struct*);
static void DestroyHashTable(void);
static void key_del(int key);
static void print_key(void);

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


static int    numberOpens = 0;
static char buffer[256] = {0};
static int buffer_len = 0;
static int32_t num;
static int args_set = 0;
static int32_t retval = -1;
static struct pb2_set_type_arguments pb2_args;
static struct obj_info heap;
static char heap_type;
static int32_t heap_size;
static int32_t topnode;

static struct file_operations file_ops =
{
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl,
};

/* Add a new process with pid to the linked list */
static void key_add(struct h_struct* entry) {
	entry->next = htable->next;
	htable->next = entry;
}

/* Return a process with a specific pic */
static struct h_struct* get_entry_from_key(int key) {
	struct h_struct* temp = htable->next;
	while (temp != NULL) {
		if (temp->key == key) {
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

/* Deletes a process entry */
static void key_del(int key) {
	struct h_struct *prev, *temp;
	prev = temp = htable;
	temp = temp->next;
	
	while (temp != NULL) {
		if (temp->key == key) {
			prev->next = temp->next;
			temp->global_heap = DestroyHeap(temp->global_heap);
			temp->next = NULL;
			printk("<key_del> Kfree key = %d", temp->key);
			kfree(temp);
			break;
		}
		prev = temp;
		temp = temp->next;
	}

}

/* Prints all the process pid */
static void print_key(void){
	struct h_struct *temp;
	temp = htable->next;
	while (temp != NULL) {
		printk("<print_key> key = %d", temp->key);
		temp = temp->next;
	}
}

/* Destroy Linked List of porcesses */
static void DestroyHashTable(void) {
	struct h_struct *temp, *temp2;
	temp = htable->next;
	htable->next = NULL;
	while (temp != NULL) {
		temp2 = temp;
		printk("<DestroyHashTable> Kfree key = %d", temp->key);
		temp = temp->next;
		kfree(temp2);
	}
	kfree(htable);
}

/* Create Heap */
static Heap *CreateHeap(int32_t capacity, int32_t heap_type) {
	Heap *h = (Heap * ) kmalloc(sizeof(Heap), GFP_KERNEL);

	//check if memory allocation is fails
	if (h == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error in allocating heap!", current->pid);
		return NULL;
	}
	h->heap_type = heap_type;
	h->count = 0;
	h->capacity = capacity;
	h->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); //size in bytes
	h->last_inserted = (int32_t)NULL;

	//check if allocation succeed
	if ( h->arr == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error while allocating heap->arr!", current->pid);
		return NULL;
	}
	return h;
}

/* Destroy Heap */
static Heap* DestroyHeap(Heap* heap) {
	if (heap == NULL)
		return -EACCES; // heap is not allocated
	printk(KERN_INFO DEVICE_NAME ": PID %d, %ld bytes of heap->arr Space freed.\n", current->pid, sizeof(heap->arr));
	kfree_const(heap->arr);
	kfree_const(heap);
	return 0;
}

/* Insert a number into heap */
static int32_t insert(Heap *h, int32_t key) {
	if ( h->count < h->capacity) {
		h->arr[h->count] = key;
		heapify_bottom_top(h, h->count);
		h->count++;
		h->last_inserted = key;
	}
	else {
		return -EACCES;  // Number of elements exceeded the capacity
	}
	return 0;
}

/* Heapify while inserting */
static void heapify_bottom_top(Heap *h, int32_t index) {
	int32_t temp;
	int32_t parent_node = (index - 1) / 2;

	if(index == 0)
		return;

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

/* Heapify while deleting */
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

/* Return the value of the top node of the heap */
static int32_t top(Heap *h) {
	int32_t top;
	if (h->count == 0) {
		printk(KERN_INFO DEVICE_NAME ": PID %d, __Heap is Empty__\n", current->pid);
		return NULL;
	}
	// replace first node by last and delete last
	top = h->arr[0];
	return top;
}

/* Extract the top node of a heap */
static int32_t PopMin(Heap *h) {
	int32_t pop;
	if (h->count == 0) {
		printk(KERN_INFO DEVICE_NAME ": PID %d, __Heap is Empty__\n", current->pid);
		return NULL;
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

	// Get the process corresponing heap
	entry = get_entry_from_key(current->pid);
	if (entry == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d RAISED ERROR in dev_write entry is non-existent", current->pid);
		return -EACCES;
	}
	// Args Set = 1 if the heap is initialized(not NULL)
	args_set = (entry->global_heap) ? 1 : 0;
	buffer_len = count < 256 ? count : 256;

	// If heap is initialized
	if (args_set) {
		if (count > 4 || count == 0) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d WRONG DATA SENT. %d bytes", current->pid, buffer_len);
			return -EINVAL;
		}
		// Check for unexpected type
		char arr[4];
		memset(arr, 0, 4 * sizeof(char));
		memcpy(arr, buf, count * sizeof(char));
		memcpy(&num, arr, sizeof(num));
		printk(DEVICE_NAME ": PID %d writing %d to heap\n", current->pid, num);

		retval = insert(entry->global_heap, num);
		if (retval < 0) { // Heap is filled to capacity
			return -EACCES;
		}
		return sizeof(num);
	}

	 // Any other call before the heap has been initialized
	if (buffer_len != 2) {
		return -EACCES;
	}

	// Initlize Heap
	heap_type = buf[0];
	heap_size = buf[1];
	printk(DEVICE_NAME ": PID %d HEAP TYPE: %d", current->pid, heap_type);
	printk(DEVICE_NAME ": PID %d HEAP SIZE: %d", current->pid, heap_size);
	printk(DEVICE_NAME ": PID %d RECIEVED:  %ld bytes", current->pid, count);

	if (heap_type != MIN_HEAP && heap_type != MAX_HEAP) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong Heap type sent!! %c\n", current->pid, heap_type);
		return -EINVAL;
	}

	if (heap_size <= 0 || heap_size > 100) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong size of heap %d!!\n", current->pid, heap_size);
		return -EINVAL;
	}

	entry->global_heap = DestroyHeap(entry->global_heap);	// destroy any existing heap before creating a new one
	entry->global_heap = CreateHeap(heap_size, heap_type);	// allocating space for new heap

	return buffer_len;
}


static ssize_t dev_read(struct file *file, char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;

	// Get the process corresponing heap
	entry = get_entry_from_key(current->pid);
	if (entry == NULL) {
		printk(KERN_ALERT DEVICE_NAME "PID %d RAISED ERROR in dev_read entry is non-existent", current->pid);
		return -EACCES;
	}
	// Args Set = 1 if the heap is initialized(not NULL)
	args_set = (entry->global_heap) ? 1 : 0;

	 // If heap is not initialized
	if (args_set == 0) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Heap not initialized", current->pid);
		return -EACCES;
	}

	// Extract the topmost node of heap
	topnode = PopMin(entry->global_heap);
	printk(DEVICE_NAME ": PID %d asking for %ld bytes\n", current->pid, count);
	retval = copy_to_user(buf, (int32_t*)&topnode, count < sizeof(topnode) ? count : sizeof(topnode));

	if (retval == 0 && topnode != -INF) {    // success!
		printk(KERN_INFO DEVICE_NAME ": PID %d Sent %ld chars with value %d to the user\n", current->pid, sizeof(topnode), topnode);
		return sizeof(topnode);
	}
	else {
		printk(KERN_INFO DEVICE_NAME ": PID %d Failed to send retval : %d, topnode is %d\n", current->pid, retval, topnode);
		return -EACCES;      // Failed -- return a bad address message (i.e. -14)
	}
}


static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	switch (cmd) {
	case PB2_SET_TYPE:
		;
		// Get the process corresponing heap
		entry = get_entry_from_key(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d RAISED ERROR in dev_ioctl:PB2_SET_TYPE entry is non-existent", current->pid);
			return -EACCES;
		}
		
		retval = copy_from_user(&pb2_args, (struct pb2_set_type_arguments *)arg, sizeof(pb2_args));
		if (retval)
			return -EINVAL;

		printk(DEVICE_NAME ": PID %d HEAP TYPE: %d", current->pid, pb2_args.heap_type);
		printk(DEVICE_NAME ": PID %d HEAP SIZE: %d", current->pid, pb2_args.heap_size);

		// Initialize heap after elementary checks
		if (pb2_args.heap_type != 0 && pb2_args.heap_type != 1){
			printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong Heap type sent!! %c\n", current->pid, heap_type);
			return -EINVAL;
		}
		if (pb2_args.heap_size <= 0 || pb2_args.heap_size > 100) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong size of heap %d!!\n", current->pid, heap_size);
			return -EINVAL;
		}

		entry->global_heap = DestroyHeap(entry->global_heap); // destroy any existing heap before creating a new one
		entry->global_heap = CreateHeap(pb2_args.heap_size, pb2_args.heap_type); // allocating space for new space

		break;

	case PB2_INSERT:
		// Get the process corresponing heap
		entry = get_entry_from_key(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME "PID %d RAISED ERROR in dev_ioctl:PB2_INSERT entry is non-existent", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the heap is initialized(not NULL)
		args_set = (entry->global_heap) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d Heap not initialized",current->pid);
			return -EACCES;
		}

		// Get the integer number and insert into heap
		retval = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(retval)
			return -EINVAL;

		printk(DEVICE_NAME ": PID %d writing %d to heap\n", current->pid, num);
		retval = insert(entry->global_heap, num);
		if (retval < 0) { // Heap is filled to capacity
			return -EACCES;
		}
		break;

	case PB2_GET_INFO:
		// Get the process corresponing heap
		entry = get_entry_from_key(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": RAISED ERROR in dev_ioctl:PB2_GET_INFO entry is non-existent %d", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the heap is initialized(not NULL)
		args_set = (entry->global_heap) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d Heap not initialized",current->pid);
			return -EACCES;
		}

		// Create a structure to send to userspace
		heap.heap_type = pb2_args.heap_type;
		heap.heap_size = entry->global_heap->count;
		heap.root = top(entry->global_heap);
		heap.last_inserted = entry->global_heap->last_inserted;

		retval = copy_to_user((struct obj_info *)arg, &heap, sizeof(struct obj_info));
		if (retval)
			return -EACCES;
		break;

	case PB2_EXTRACT:
		// Get the process corresponing heap
		entry = get_entry_from_key(current->pid);
		if (entry == NULL) {
			printk(KERN_ALERT DEVICE_NAME ": RAISED ERROR in dev_ioctl:PB2_EXTRACT entry is non-existent %d", current->pid);
			return -EACCES;
		}
		// Args Set = 1 if the heap is initialized(not NULL)
		args_set = (entry->global_heap) ? 1 : 0;
		if (args_set == 0) {
			printk(KERN_ALERT DEVICE_NAME " : PID %d Heap not initialized",current->pid);
			return -EACCES;
		}
		
		// Create Structure to send to userspace
		struct result res =
		{
			.result = PopMin(entry->global_heap),
			.heap_size = entry->global_heap->count,
		};

		retval = copy_to_user((struct result *)arg, &res, sizeof(struct result));
		if (retval != 0 || res.result == NULL){
			printk(KERN_INFO DEVICE_NAME ": PID %d Failed to send retval : %d, topnode is %d\n", current->pid, retval, res.result);
			return -EACCES;
		}
		break;

	default:
		return -EINVAL;
	}
	return 0;
}


static int dev_open(struct inode *inodep, struct file *filep) {
	// If same process has already opened the file
	if (get_entry_from_key(current->pid) != NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d, Tried to open twice\n", current->pid);
		return -EACCES;
	}

	// Create a new entry to the process linked list
	entry = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
	*entry = (struct h_struct) {current->pid, NULL, NULL};
	if (!mutex_trylock(&heap_mutex)) {
		printk(KERN_ALERT DEVICE_NAME "PID %d Device is in Use <dev_open>", current->pid);
		return -EBUSY;
	}
	printk(DEVICE_NAME ": PID %d !!!! Adding %d to HashTable\n", current->pid, entry->key);
	key_add(entry);

	numberOpens++;
	printk(KERN_INFO DEVICE_NAME ": PID %d Device has been opened %d time(s)\n", current->pid, numberOpens);
	print_key();
	mutex_unlock(&heap_mutex);
	return 0;
}


static int dev_release(struct inode *inodep, struct file *filep) {
	if (!mutex_trylock(&heap_mutex)) {
		printk(KERN_ALERT DEVICE_NAME "PID %d Device is in Use <dev_release>.", current->pid);
		return -EBUSY;
	}
	// Delete the process entry from the process linked list
	key_del(current->pid);
	printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully closed\n", current->pid);
	print_key();
	mutex_unlock(&heap_mutex);
	return 0;
}


static int hello_init(void) {
	struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0, NULL, &file_ops);
	if (!entry)
		return -ENOENT;
	htable = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
	*htable = (struct h_struct) { -1, NULL, NULL};
	printk(KERN_ALERT DEVICE_NAME ": Hello world\n");
	mutex_init(&heap_mutex);	
	return 0;
}

static void hello_exit(void) {
	mutex_destroy(&heap_mutex);
	DestroyHashTable();
	remove_proc_entry(DEVICE_NAME, NULL);

	printk(KERN_ALERT DEVICE_NAME "Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);

