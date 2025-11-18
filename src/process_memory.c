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
#define PROCESS_STACK_START   0x0FFFF000  /* Just below 256 MB */
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
    paging_map_page_dir(proc->memory.page_directory, PROCESS_CODE_START,
                        (uint32_t)code_page, PAGE_PRESENT | PAGE_WRITE);

    /* Allocate one page for data (BONUS) */
    void *data_page = vmalloc(PAGE_SIZE);
    if (data_page) {
        paging_map_page_dir(proc->memory.page_directory, PROCESS_DATA_START,
                            (uint32_t)data_page, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Allocate one page for BSS (BONUS) */
    void *bss_page = vmalloc(PAGE_SIZE);
    if (bss_page) {
        paging_map_page_dir(proc->memory.page_directory, PROCESS_BSS_START,
                            (uint32_t)bss_page, PAGE_PRESENT | PAGE_WRITE);
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
    paging_map_page_dir(proc->memory.page_directory, PROCESS_HEAP_START,
                        (uint32_t)heap_page, PAGE_PRESENT | PAGE_WRITE);

    /* Allocate stack */
    void *stack_page = vmalloc(PROCESS_STACK_SIZE);
    if (!stack_page) {
        printk("[PROCESS] Failed to allocate stack\n");
        vfree(code_page);
        vfree(heap_page);
        return -1;
    }

    /* Map stack (grows downward) - map all pages */
    uint32_t stack_base = PROCESS_STACK_START - PROCESS_STACK_SIZE;
    uint32_t num_stack_pages = PROCESS_STACK_SIZE / PAGE_SIZE;
    for (uint32_t i = 0; i < num_stack_pages; i++) {
        uint32_t virt_addr = stack_base + (i * PAGE_SIZE);
        uint32_t phys_addr = (uint32_t)stack_page + (i * PAGE_SIZE);
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
        paging_map_page_dir(dest->memory.page_directory, dest->memory.code_start,
                            (uint32_t)code_page, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy data (BONUS) */
    void *data_page = vmalloc(PAGE_SIZE);
    if (data_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.data_start);
        if (src_phys) {
            memcpy(data_page, (void*)src_phys, PAGE_SIZE);
        }
        paging_map_page_dir(dest->memory.page_directory, dest->memory.data_start,
                            (uint32_t)data_page, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy BSS (BONUS) */
    void *bss_page = vmalloc(PAGE_SIZE);
    if (bss_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.bss_start);
        if (src_phys) {
            memcpy(bss_page, (void*)src_phys, PAGE_SIZE);
        }
        paging_map_page_dir(dest->memory.page_directory, dest->memory.bss_start,
                            (uint32_t)bss_page, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy heap */
    void *heap_page = vmalloc(PAGE_SIZE);
    if (heap_page) {
        uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                             src->memory.heap_start);
        if (src_phys) {
            memcpy(heap_page, (void*)src_phys, PAGE_SIZE);
        }
        paging_map_page_dir(dest->memory.page_directory, dest->memory.heap_start,
                            (uint32_t)heap_page, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Copy stack */
    void *stack_page = vmalloc(PROCESS_STACK_SIZE);
    if (stack_page) {
        uint32_t stack_base = PROCESS_STACK_START - PROCESS_STACK_SIZE;
        uint32_t num_stack_pages = PROCESS_STACK_SIZE / PAGE_SIZE;

        /* Copy each page of the stack */
        for (uint32_t i = 0; i < num_stack_pages; i++) {
            uint32_t virt_addr = stack_base + (i * PAGE_SIZE);
            uint32_t src_phys = paging_get_physical_address_dir(src->memory.page_directory,
                                                                 virt_addr);
            if (src_phys) {
                void *dest_page = (void*)((uint32_t)stack_page + (i * PAGE_SIZE));
                memcpy(dest_page, (void*)src_phys, PAGE_SIZE);
            }

            /* Map each page in destination */
            uint32_t phys_addr = (uint32_t)stack_page + (i * PAGE_SIZE);
            paging_map_page_dir(dest->memory.page_directory, virt_addr,
                                phys_addr, PAGE_PRESENT | PAGE_WRITE);
        }
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
