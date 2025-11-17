/* kmalloc.c - Physical memory allocator implementation */

#include "../include/kmalloc.h"
#include "../include/panic.h"
#include "../include/printf.h"
#include "../include/string.h"

/* Kernel heap configuration */
#define HEAP_START 0x00500000  /* Start at 5MB */
#define HEAP_SIZE  0x00100000  /* 1MB heap */
#define HEAP_END   (HEAP_START + HEAP_SIZE)

/* Block header for memory allocations */
typedef struct mem_block {
    size_t size;              /* Size of the block (including header) */
    bool is_free;             /* Is this block free? */
    struct mem_block *next;   /* Next block in list */
    uint32_t magic;           /* Magic number for validation */
} mem_block_t;

#define BLOCK_MAGIC 0xDEADBEEF
#define BLOCK_HEADER_SIZE sizeof(mem_block_t)

/* Head of the free list */
static mem_block_t *heap_start = NULL;
static bool heap_initialized = false;

/* Statistics */
static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t num_allocations = 0;

/* Initialize kernel memory allocator */
void kmalloc_init(void) {
    if (heap_initialized) {
        return;
    }

    /* Initialize the heap with one large free block */
    heap_start = (mem_block_t *)HEAP_START;
    heap_start->size = HEAP_SIZE;
    heap_start->is_free = true;
    heap_start->next = NULL;
    heap_start->magic = BLOCK_MAGIC;

    heap_initialized = true;

    kernel_info("Kernel heap initialized");
    printk("  Heap start: 0x%x\n", HEAP_START);
    printk("  Heap size:  %d KB\n", HEAP_SIZE / 1024);
}

/* Find a free block that fits the requested size */
static mem_block_t *find_free_block(size_t size) {
    mem_block_t *current = heap_start;

    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }

    return NULL; /* No suitable block found */
}

/* Split a block if it's too large */
static void split_block(mem_block_t *block, size_t size) {
    /* Only split if there's enough space for a new block */
    if (block->size >= size + BLOCK_HEADER_SIZE + 16) {
        mem_block_t *new_block = (mem_block_t *)((uint32_t)block + size);
        new_block->size = block->size - size;
        new_block->is_free = true;
        new_block->next = block->next;
        new_block->magic = BLOCK_MAGIC;

        block->size = size;
        block->next = new_block;
    }
}

/* Merge adjacent free blocks */
static void merge_free_blocks(void) {
    mem_block_t *current = heap_start;

    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            /* Merge current with next */
            current->size += current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

/* Allocate physical memory */
void *kmalloc(size_t size) {
    if (!heap_initialized) {
        kmalloc_init();
    }

    if (size == 0) {
        return NULL;
    }

    /* Align size to 4 bytes and add header */
    size = (size + 3) & ~3;
    size_t total_size = size + BLOCK_HEADER_SIZE;

    /* Find a free block */
    mem_block_t *block = find_free_block(total_size);

    if (block == NULL) {
        kernel_warning("kmalloc: Out of memory");
        return NULL;
    }

    /* Split the block if needed */
    split_block(block, total_size);

    /* Mark as allocated */
    block->is_free = false;

    /* Update statistics */
    total_allocated += size;
    num_allocations++;

    /* Return pointer to data (after header) */
    return (void *)((uint32_t)block + BLOCK_HEADER_SIZE);
}

/* Allocate aligned physical memory */
void *kmalloc_aligned(size_t size, uint32_t align) {
    /* Simple implementation: just allocate extra space */
    void *ptr = kmalloc(size + align);
    if (!ptr) {
        return NULL;
    }

    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + align - 1) & ~(align - 1);

    return (void *)aligned_addr;
}

/* Free physical memory */
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }

    /* Get block header */
    mem_block_t *block = (mem_block_t *)((uint32_t)ptr - BLOCK_HEADER_SIZE);

    /* Validate magic number */
    if (block->magic != BLOCK_MAGIC) {
        kernel_panic("kfree: Invalid pointer or corrupted heap");
    }

    if (block->is_free) {
        kernel_warning("kfree: Double free detected");
        return;
    }

    /* Mark as free */
    block->is_free = true;

    /* Update statistics */
    total_freed += (block->size - BLOCK_HEADER_SIZE);

    /* Merge adjacent free blocks */
    merge_free_blocks();
}

/* Get size of allocated block */
size_t ksize(void *ptr) {
    if (!ptr) {
        return 0;
    }

    /* Get block header */
    mem_block_t *block = (mem_block_t *)((uint32_t)ptr - BLOCK_HEADER_SIZE);

    /* Validate magic number */
    if (block->magic != BLOCK_MAGIC) {
        return 0;
    }

    return block->size - BLOCK_HEADER_SIZE;
}

/* Get kernel heap statistics */
void kmalloc_stats(void) {
    printk("\n=== Kernel Heap Statistics ===\n");
    printk("Heap start:       0x%x\n", HEAP_START);
    printk("Heap size:        %d KB\n", HEAP_SIZE / 1024);
    printk("Total allocated:  %d bytes\n", total_allocated);
    printk("Total freed:      %d bytes\n", total_freed);
    printk("Currently used:   %d bytes\n", total_allocated - total_freed);
    printk("Allocations:      %d\n", num_allocations);

    /* Count free blocks */
    int free_blocks = 0;
    size_t free_memory = 0;
    mem_block_t *current = heap_start;

    while (current != NULL) {
        if (current->is_free) {
            free_blocks++;
            free_memory += current->size - BLOCK_HEADER_SIZE;
        }
        current = current->next;
    }

    printk("Free blocks:      %d\n", free_blocks);
    printk("Free memory:      %d bytes\n", free_memory);
    printk("\n");
}
