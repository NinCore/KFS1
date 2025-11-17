/* paging.c - Memory paging implementation */

#include "../include/paging.h"
#include "../include/panic.h"
#include "../include/printf.h"
#include "../include/string.h"

/* Kernel page directory (must be page-aligned) */
static page_directory_t kernel_directory __attribute__((aligned(PAGE_SIZE)));

/* Kernel page tables (identity map first 4MB) */
static page_table_t kernel_table_0 __attribute__((aligned(PAGE_SIZE)));

/* Current page directory */
static page_directory_t *current_directory = NULL;

/* Initialize paging */
void paging_init(void) {
    /* Clear the page directory */
    memset(&kernel_directory, 0, sizeof(page_directory_t));

    /* Clear the first page table */
    memset(&kernel_table_0, 0, sizeof(page_table_t));

    /* Identity map the first 4MB (0x00000000 - 0x00400000) */
    /* This ensures kernel code and data remain accessible */
    for (uint32_t i = 0; i < PAGE_ENTRIES; i++) {
        uint32_t phys_addr = i * PAGE_SIZE;
        kernel_table_0.entries[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    }

    /* Set the first page directory entry to point to our page table */
    kernel_directory.entries[0] = ((uint32_t)&kernel_table_0) | PAGE_PRESENT | PAGE_WRITE;

    /* Set as current directory */
    current_directory = &kernel_directory;

    kernel_info("Paging initialized (identity mapped first 4MB)");
}

/* Enable paging by loading CR3 and setting the paging bit in CR0 */
void paging_enable(void) {
    if (!current_directory) {
        kernel_panic("Cannot enable paging: no page directory set");
    }

    /* Load page directory address into CR3 */
    __asm__ volatile("mov %0, %%cr3" : : "r"(&kernel_directory));

    /* Enable paging by setting bit 31 in CR0 */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; /* Set PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    kernel_info("Paging enabled");
}

/* Get the current page directory */
page_directory_t *paging_get_directory(void) {
    return current_directory;
}

/* Map a virtual address to a physical address */
void paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    /* Extract directory and table indices from virtual address */
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    /* Check if page directory entry exists */
    if (!(current_directory->entries[dir_index] & PAGE_PRESENT)) {
        /* Would need to allocate a new page table here */
        /* For now, we only support the first 4MB */
        kernel_panic("Cannot map page: page table not present");
    }

    /* Get the page table */
    page_table_t *table = (page_table_t *)(current_directory->entries[dir_index] & ~0xFFF);

    /* Set the page table entry */
    table->entries[table_index] = (phys_addr & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;

    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

/* Unmap a virtual address */
void paging_unmap_page(uint32_t virt_addr) {
    /* Extract directory and table indices */
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    /* Check if page directory entry exists */
    if (!(current_directory->entries[dir_index] & PAGE_PRESENT)) {
        return; /* Already unmapped */
    }

    /* Get the page table */
    page_table_t *table = (page_table_t *)(current_directory->entries[dir_index] & ~0xFFF);

    /* Clear the page table entry */
    table->entries[table_index] = 0;

    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

/* Get physical address from virtual address */
uint32_t paging_get_physical_address(uint32_t virt_addr) {
    /* Extract directory and table indices */
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;
    uint32_t offset = virt_addr & 0xFFF;

    /* Check if page directory entry exists */
    if (!(current_directory->entries[dir_index] & PAGE_PRESENT)) {
        return 0; /* Not mapped */
    }

    /* Get the page table */
    page_table_t *table = (page_table_t *)(current_directory->entries[dir_index] & ~0xFFF);

    /* Check if page table entry exists */
    if (!(table->entries[table_index] & PAGE_PRESENT)) {
        return 0; /* Not mapped */
    }

    /* Return physical address */
    return (table->entries[table_index] & ~0xFFF) | offset;
}

/* Page fault handler (called from ISR) */
void page_fault_handler(void) {
    /* Get the faulting address from CR2 */
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));

    /* Get error code (would be passed as parameter in full implementation) */
    printk("\nPage fault at address: 0x%x\n", faulting_address);

    kernel_panic("Page fault");
}
