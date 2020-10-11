/*
PART B: Assignment 1
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

#define DEVICE_NAME "partb_1_16CS30031"
#define INF  (0xffffffff)
#define current get_current()
const char MIN_HEAP = 0xFF, MAX_HEAP = 0xF0;

MODULE_LICENSE("GPL");
static DEFINE_MUTEX(heap_mutex);

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
static Heap *CreateHeap(int32_t capacity, char heap_type);
static Heap* DestroyHeap(Heap* heap);
static int32_t insert(Heap *h, int32_t key);
static void heapify_bottom_top(Heap *h, int32_t index);
static void heapify_top_bottom(Heap *h, int32_t parent_node);
static int32_t PopMin(Heap *h);

static struct h_struct* get_entry_from_key(int key);
static void key_add(struct h_struct*);
static void DestroyHashTable(void);
static void key_del(int key);
static void print_key(void);

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


static int    numberOpens = 0;
static char buffer[256] = {0};
static int buffer_len = 0;
static int args_set = 0;
static int retval = -1;
static int32_t num ;
static int32_t ret;
static char heap_type;
static int32_t heap_size;
static int32_t topnode;

static struct file_operations file_ops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};


static void key_add(struct h_struct* entry) {
	entry->next = htable->next;
	htable->next = entry;
}


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


static void key_del(int key) {
	struct h_struct *prev, *temp;
	prev = temp = htable;
	temp = temp->next;

	while (temp != NULL) {
		if (temp->key == key) {
			prev->next = temp->next;
			temp->global_heap = DestroyHeap(temp->global_heap);
			temp->next = NULL;
			printk("PID %d <key_del> Kfree key = %d", current->pid, temp->key);
			kfree(temp);
			break;
		}
		prev = temp;
		temp = temp->next;
	}

}


static void print_key(void) {
	struct h_struct *temp;
	temp = htable->next;
	while (temp != NULL) {
		printk("<print_key> key = %d", temp->key);
		temp = temp->next;
	}
}


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


static Heap *CreateHeap(int32_t capacity, char heap_type) {
	Heap *h = (Heap * ) kmalloc(sizeof(Heap), GFP_KERNEL); //one is number of heap

	//check if memory allocation is fails
	if (h == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error in allocating heap!", current->pid);
		return NULL;
	}
	h->heap_type = ((heap_type == MIN_HEAP) ? 0 : 1);
	h->count = 0;
	h->capacity = capacity;
	h->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); //size in bytes

	//check if allocation succeed
	if ( h->arr == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error while allocating heap->arr!", current->pid);
		return NULL;
	}
	return h;
}


static Heap* DestroyHeap(Heap* heap) {
	if (heap == NULL)
		return NULL; // heap is not allocated
	printk(KERN_INFO DEVICE_NAME ": PID %d, %ld bytes of heap->arr Space freed.\n", current->pid, sizeof(heap->arr));
	kfree_const(heap->arr);
	kfree_const(heap);
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

	if (h->heap_type == 0) {	// Min Heap
		if (h->arr[parent_node] > h->arr[index]) {
			//swap and recursive call
			temp = h->arr[parent_node];
			h->arr[parent_node] = h->arr[index];
			h->arr[index] = temp;
			heapify_bottom_top(h, parent_node);
		}
	}
	else {	// Max Heap
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

	if (h->heap_type == 0) {	// Min heap
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
	else {	// Max heap
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


static int32_t PopMin(Heap *h) {
	int32_t pop;
	if (h->count == 0) {
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


	entry = get_entry_from_key(current->pid);
	if (entry == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d RAISED ERROR in dev_write entry is non-existent", current->pid);
		return -EACCES;
	}

	args_set = (entry->global_heap) ? 1 : 0;
	buffer_len = count < 256 ? count : 256;


	if (args_set) {
		if (count > 4 || count == 0) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d WRONG DATA SENT. %d bytes", current->pid, buffer_len);
			return -EINVAL;
		}
		char arr[4];
		memset(arr, 0, 4 * sizeof(char));
		memcpy(arr, buf, count * sizeof(char));
		memcpy(&num, arr, sizeof(num));
		printk(DEVICE_NAME ": PID %d writing %d to heap\n", current->pid, num);

		ret = insert(entry->global_heap, num);
		if (ret < 0) { // Heap is filled to capacity
			return -EACCES;
		}
		return sizeof(num);
	}


	if (buffer_len != 2) { // any other call before the heap has been initialized
		return -EACCES;
	}


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


	entry = get_entry_from_key(current->pid);
	if (entry == NULL) {
		printk(KERN_ALERT DEVICE_NAME "PID %d RAISED ERROR in dev_read entry is non-existent", current->pid);
		return -EACCES;
	}
	args_set = (entry->global_heap) ? 1 : 0;

	if (args_set == 0) { // heap hasn't been initialized yet
		printk(KERN_ALERT DEVICE_NAME ": PID %d Heap not initialized", current->pid);
		return -EACCES;
	}
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


static int dev_open(struct inode *inodep, struct file *filep) {
	if (get_entry_from_key(current->pid) != NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d, Tried to open twice\n", current->pid);
		return -EACCES;
	}

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

	printk(KERN_ALERT DEVICE_NAME ": Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);

