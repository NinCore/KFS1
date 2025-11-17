/* vmalloc.c - Virtual memory allocator implementation */

#include "../include/vmalloc.h"
#include "../include/kmalloc.h"
#include "../include/paging.h"
#include "../include/panic.h"
#include "../include/printf.h"
#include "../include/string.h"

/* Virtual memory configuration */
#define VMEM_START 0x10000000  /* Start at 256MB */
#define VMEM_SIZE  0x10000000  /* 256MB virtual space */
#define VMEM_END   (VMEM_START + VMEM_SIZE)

/* Virtual memory block header */
typedef struct vmem_block {
    size_t size;                /* Size of the block */
    bool is_free;               /* Is this block free? */
    uint32_t virt_addr;         /* Virtual address */
    uint32_t phys_addr;         /* Physical address (if mapped) */
    struct vmem_block *next;    /* Next block */
    uint32_t magic;             /* Magic number */
} vmem_block_t;

#define VMEM_MAGIC 0xCAFEBABE
#define VMEM_HEADER_SIZE sizeof(vmem_block_t)

/* Head of virtual memory list */
static vmem_block_t *vmem_head = NULL;
static bool vmem_initialized = false;
static uint32_t next_virt_addr = VMEM_START;

/* Statistics */
static size_t vmem_allocated = 0;
static size_t vmem_freed = 0;

/* Initialize virtual memory allocator */
void vmalloc_init(void) {
    if (vmem_initialized) {
        return;
    }

    /* Allocate initial block list from kernel heap */
    vmem_head = (vmem_block_t *)kmalloc(VMEM_HEADER_SIZE);
    if (!vmem_head) {
        kernel_panic("Failed to initialize virtual memory allocator");
    }

    vmem_head->size = VMEM_SIZE;
    vmem_head->is_free = true;
    vmem_head->virt_addr = VMEM_START;
    vmem_head->phys_addr = 0;
    vmem_head->next = NULL;
    vmem_head->magic = VMEM_MAGIC;

    vmem_initialized = true;
    kernel_info("Virtual memory allocator initialized");
}

/* Find free virtual memory block */
static vmem_block_t *find_free_vmem_block(size_t size) {
    vmem_block_t *current = vmem_head;

    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/* Allocate virtual memory */
void *vmalloc(size_t size) {
    if (!vmem_initialized) {
        vmalloc_init();
    }

    if (size == 0) {
        return NULL;
    }

    /* Align to page boundaries */
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /* Find free virtual memory space */
    vmem_block_t *block = find_free_vmem_block(size);

    if (!block) {
        kernel_warning("vmalloc: Out of virtual memory");
        return NULL;
    }

    /* Allocate physical pages for this virtual memory */
    uint32_t num_pages = size / PAGE_SIZE;
    uint32_t virt_addr = block->virt_addr;

    for (uint32_t i = 0; i < num_pages; i++) {
        /* Allocate a physical page */
        void *phys_page = kmalloc(PAGE_SIZE);
        if (!phys_page) {
            kernel_warning("vmalloc: Failed to allocate physical memory");
            return NULL;
        }

        /* Map virtual to physical */
        uint32_t virt_page = virt_addr + (i * PAGE_SIZE);
        /* Note: paging_map_page would be called here in full implementation */
        /* For now, we just track the allocation */
    }

    /* Mark as allocated */
    block->is_free = false;
    block->size = size;

    /* Split remaining space if needed */
    if (block->size < VMEM_SIZE) {
        vmem_block_t *new_block = (vmem_block_t *)kmalloc(VMEM_HEADER_SIZE);
        if (new_block) {
            new_block->size = VMEM_SIZE - size;
            new_block->is_free = true;
            new_block->virt_addr = virt_addr + size;
            new_block->phys_addr = 0;
            new_block->next = block->next;
            new_block->magic = VMEM_MAGIC;
            block->next = new_block;
        }
    }

    vmem_allocated += size;

    return (void *)virt_addr;
}

/* Free virtual memory */
void vfree(void *ptr) {
    if (!ptr) {
        return;
    }

    uint32_t addr = (uint32_t)ptr;

    /* Find the block */
    vmem_block_t *current = vmem_head;
    while (current != NULL) {
        if (current->virt_addr == addr && !current->is_free) {
            /* Mark as free */
            current->is_free = true;
            vmem_freed += current->size;

            /* TODO: Unmap pages and free physical memory */

            /* Merge adjacent free blocks */
            vmem_block_t *scan = vmem_head;
            while (scan != NULL && scan->next != NULL) {
                if (scan->is_free && scan->next->is_free) {
                    vmem_block_t *temp = scan->next;
                    scan->size += temp->size;
                    scan->next = temp->next;
                    kfree(temp);
                } else {
                    scan = scan->next;
                }
            }

            return;
        }
        current = current->next;
    }

    kernel_warning("vfree: Invalid pointer");
}

/* Get size of allocated virtual block */
size_t vsize(void *ptr) {
    if (!ptr) {
        return 0;
    }

    uint32_t addr = (uint32_t)ptr;

    vmem_block_t *current = vmem_head;
    while (current != NULL) {
        if (current->virt_addr == addr) {
            return current->size;
        }
        current = current->next;
    }

    return 0;
}

/* Get virtual memory statistics */
void vmalloc_stats(void) {
    printk("\n=== Virtual Memory Statistics ===\n");
    printk("Virtual start:    0x%x\n", VMEM_START);
    printk("Virtual size:     %d MB\n", VMEM_SIZE / (1024 * 1024));
    printk("Total allocated:  %d bytes\n", vmem_allocated);
    printk("Total freed:      %d bytes\n", vmem_freed);
    printk("Currently used:   %d bytes\n", vmem_allocated - vmem_freed);

    /* Count blocks */
    int total_blocks = 0;
    int free_blocks = 0;
    vmem_block_t *current = vmem_head;

    while (current != NULL) {
        total_blocks++;
        if (current->is_free) {
            free_blocks++;
        }
        current = current->next;
    }

    printk("Total blocks:     %d\n", total_blocks);
    printk("Free blocks:      %d\n", free_blocks);
    printk("\n");
}
