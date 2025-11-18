/* process_memory.c - Process memory management */

#include "../include/process.h"
#include "../include/paging.h"
#include "../include/kmalloc.h"
#include "../include/vmalloc.h"
#include "../include/string.h"
#include "../include/printf.h"

/* Memory layout constants */
#define PROCESS_CODE_START    0x08000000  /* 128 MB */
#define PROCESS_DATA_START    0x09000000  /* 144 MB */
#define PROCESS_BSS_START     0x0A000000  /* 160 MB */
#define PROCESS_HEAP_START    0x0B000000  /* 176 MB */
#define PROCESS_STACK_START   0x10000000  /* 256 MB - top of stack */
#define PROCESS_STACK_SIZE    0x00010000  /* 64 KB stack */

/* Initialize process memory */
int process_memory_init(struct process *proc) {
    if (!proc) {
        return -1;
    }

    /* Create new page directory for process */
    proc->memory.page_directory = paging_create_directory();
    if (!proc->memory.page_directory) {
        printk("[PROCESS] Failed to create page directory\n");
        return -1;
    }

    /* Set up memory regions */
    proc->memory.code_start = PROCESS_CODE_START;
    proc->memory.code_end = PROCESS_CODE_START;

    /* BONUS: Data and BSS sections */
    proc->memory.data_start = PROCESS_DATA_START;
    proc->memory.data_end = PROCESS_DATA_START;
    proc->memory.bss_start = PROCESS_BSS_START;
    proc->memory.bss_end = PROCESS_BSS_START;

    proc->memory.heap_start = PROCESS_HEAP_START;
    proc->memory.heap_end = PROCESS_HEAP_START;

    proc->memory.stack_start = PROCESS_STACK_START;
    proc->memory.stack_end = PROCESS_STACK_START;

    /* Allocate one page for code */
    void *code_page = vmalloc(PAGE_SIZE);
    if (!code_page) {
        printk("[PROCESS] Failed to allocate code page\n");
        return -1;
    }
    uint32_t code_phys = paging_get_physical_address((uint32_t)code_page);
    paging_map_page_dir(proc->memory.page_directory, PROCESS_CODE_START,
                        code_phys, PAGE_PRESENT | PAGE_WRITE);

    /* Allocate one page for data (BONUS) */
    void *data_page = vmalloc(PAGE_SIZE);
    if (data_page) {
        uint32_t data_phys = paging_get_physical_address((uint32_t)data_page);
        paging_map_page_dir(proc->memory.page_directory, PROCESS_DATA_START,
                            data_phys, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Allocate one page for BSS (BONUS) */
    void *bss_page = vmalloc(PAGE_SIZE);
    if (bss_page) {
        uint32_t bss_phys = paging_get_physical_address((uint32_t)bss_page);
        paging_map_page_dir(proc->memory.page_directory, PROCESS_BSS_START,
                            bss_phys, PAGE_PRESENT | PAGE_WRITE);
        /* Clear BSS */
        memset(bss_page, 0, PAGE_SIZE);
    }

    /* Allocate one page for heap */
    void *heap_page = vmalloc(PAGE_SIZE);
    if (!heap_page) {
        printk("[PROCESS] Failed to allocate heap page\n");
        vfree(code_page);
        return -1;
    }
    uint32_t heap_phys = paging_get_physical_address((uint32_t)heap_page);
    paging_map_page_dir(proc->memory.page_directory, PROCESS_HEAP_START,
                        heap_phys, PAGE_PRESENT | PAGE_WRITE);

    /* Allocate stack - use kmalloc to get physical pages */
    /* Map stack (grows downward) - allocate and map all pages */
    uint32_t stack_base = PROCESS_STACK_START - PROCESS_STACK_SIZE;
    uint32_t num_stack_pages = PROCESS_STACK_SIZE / PAGE_SIZE;
    for (uint32_t i = 0; i < num_stack_pages; i++) {
        /* Allocate one physical page */
        void *phys_page = kmalloc(PAGE_SIZE);
        if (!phys_page) {
            printk("[PROCESS] Failed to allocate stack page %d\n", i);
            vfree(code_page);
            vfree(heap_page);
            return -1;
        }

        /* With identity mapping, kmalloc returns addresses that are both virtual and physical */
        /* Get physical address (same as virtual for identity-mapped kernel) */
        uint32_t phys_addr = (uint32_t)phys_page;

        /* Map this page into process space */
        uint32_t virt_addr = stack_base + (i * PAGE_SIZE);
        paging_map_page_dir(proc->memory.page_directory, virt_addr,
                            phys_addr, PAGE_PRESENT | PAGE_WRITE);
    }

    printk("[PROCESS] Memory initialized for PID %d\n", proc->pid);
    printk("  Code:  0x%x - 0x%x\n", proc->memory.code_start, proc->memory.code_end);
    printk("  Data:  0x%x - 0x%x\n", proc->memory.data_start, proc->memory.data_end);
    printk("  BSS:   0x%x - 0x%x\n", proc->memory.bss_start, proc->memory.bss_end);
    printk("  Heap:  0x%x - 0x%x\n", proc->memory.heap_start, proc->memory.heap_end);
    printk("  Stack: 0x%x - 0x%x\n", stack_base, PROCESS_STACK_START);

    return 0;
}

/* Copy process memory (for fork) */
int process_memory_copy(struct process *dest, struct process *src) {
    if (!dest || !src) {
        return -1;
    }

    /* Create new page directory */
    dest->memory.page_directory = paging_create_directory();
    if (!dest->memory.page_directory) {
        return -1;
    }

    /* Copy memory layout */
    dest->memory = src->memory;
    dest->memory.page_directory = dest->memory.page_directory;

    /* Copy code */
    void *code_page = vmalloc(PAGE_SIZE);
    if (code_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.code_start);
        if (src_phys) {
            memcpy(code_page, (void*)src_phys, PAGE_SIZE);
        }
        uint32_t code_phys = paging_get_physical_address((uint32_t)code_page);
        paging_map_page_dir(dest->memory.page_directory, dest->memory.code_start,
                            code_phys, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy data (BONUS) */
    void *data_page = vmalloc(PAGE_SIZE);
    if (data_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.data_start);
        if (src_phys) {
            memcpy(data_page, (void*)src_phys, PAGE_SIZE);
        }
        uint32_t data_phys = paging_get_physical_address((uint32_t)data_page);
        paging_map_page_dir(dest->memory.page_directory, dest->memory.data_start,
                            data_phys, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy BSS (BONUS) */
    void *bss_page = vmalloc(PAGE_SIZE);
    if (bss_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.bss_start);
        if (src_phys) {
            memcpy(bss_page, (void*)src_phys, PAGE_SIZE);
        }
        uint32_t bss_phys = paging_get_physical_address((uint32_t)bss_page);
        paging_map_page_dir(dest->memory.page_directory, dest->memory.bss_start,
                            bss_phys, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy heap */
    void *heap_page = vmalloc(PAGE_SIZE);
    if (heap_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.heap_start);
        if (src_phys) {
            memcpy(heap_page, (void*)src_phys, PAGE_SIZE);
        }
        uint32_t heap_phys = paging_get_physical_address((uint32_t)heap_page);
        paging_map_page_dir(dest->memory.page_directory, dest->memory.heap_start,
                            heap_phys, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy stack - allocate each page with kmalloc */
    uint32_t stack_base = PROCESS_STACK_START - PROCESS_STACK_SIZE;
    uint32_t num_stack_pages = PROCESS_STACK_SIZE / PAGE_SIZE;

    /* Copy each page of the stack */
    for (uint32_t i = 0; i < num_stack_pages; i++) {
        uint32_t virt_addr = stack_base + (i * PAGE_SIZE);
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             virt_addr);

        /* Allocate new physical page for destination */
        void *phys_page = kmalloc(PAGE_SIZE);
        if (!phys_page) {
            continue; /* Skip this page if allocation fails */
        }

        /* Copy data from source if it exists */
        if (src_phys) {
            memcpy(phys_page, (void*)src_phys, PAGE_SIZE);
        }

        /* Map page in destination process */
        uint32_t phys_addr = (uint32_t)phys_page;
        paging_map_page_dir(dest->memory.page_directory, virt_addr,
                            phys_addr, PAGE_PRESENT | PAGE_WRITE);
    }

    return 0;
}

/* Free process memory */
void process_memory_free(struct process *proc) {
    if (!proc || !proc->memory.page_directory) {
        return;
    }

    /* Free all allocated pages */
    /* TODO: Walk page directory and free all mapped pages */

    /* Free page directory */
    paging_destroy_directory(proc->memory.page_directory);
    proc->memory.page_directory = NULL;
}

/* BONUS: mmap implementation */
void *process_mmap(struct process *proc, void *addr, size_t length, int prot, int flags) {
    if (!proc) {
        return NULL;
    }

    (void)prot;  /* TODO: Use protection flags */
    (void)flags; /* TODO: Use mapping flags */

    /* Align length to page size */
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t total_size = pages * PAGE_SIZE;

    /* Allocate virtual memory */
    void *virt_addr = vmalloc(total_size);
    if (!virt_addr) {
        return NULL;
    }

    /* If addr is NULL, choose address in heap area */
    uint32_t map_addr = addr ? (uint32_t)addr : proc->memory.heap_end;

    /* Map pages */
    for (size_t i = 0; i < pages; i++) {
        uint32_t virt = map_addr + (i * PAGE_SIZE);
        uint32_t phys = (uint32_t)virt_addr + (i * PAGE_SIZE);
        paging_map_page_dir(proc->memory.page_directory, virt, phys,
                            PAGE_PRESENT | PAGE_WRITE);
    }

    /* Update heap end if mapping in heap area */
    if (!addr) {
        proc->memory.heap_end = map_addr + total_size;
    }

    return (void*)map_addr;
}

/* BONUS: munmap implementation */
int process_munmap(struct process *proc, void *addr, size_t length) {
    if (!proc || !addr) {
        return -1;
    }

    /* Align length to page size */
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Unmap pages */
    for (size_t i = 0; i < pages; i++) {
        uint32_t virt = (uint32_t)addr + (i * PAGE_SIZE);
        paging_unmap_page_dir(proc->memory.page_directory, virt);
    }

    return 0;
}
