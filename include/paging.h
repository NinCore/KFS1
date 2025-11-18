/* paging.h - Memory paging structures and functions */

#ifndef PAGING_H
#define PAGING_H

#include "types.h"

/* Page size (4KB) */
#define PAGE_SIZE 4096

/* Number of entries in page directory and page table */
#define PAGE_ENTRIES 1024

/* Page directory and table entry flags */
#define PAGE_PRESENT    0x1   /* Page is present in memory */
#define PAGE_WRITE      0x2   /* Page is writable */
#define PAGE_USER       0x4   /* Page is accessible from user mode */
#define PAGE_ACCESSED   0x20  /* Page was accessed */
#define PAGE_DIRTY      0x40  /* Page was written to */

/* Page directory entry */
typedef uint32_t page_directory_entry_t;

/* Page table entry */
typedef uint32_t page_table_entry_t;

/* Page directory (1024 entries) */
typedef struct page_directory {
    page_directory_entry_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

/* Page table (1024 entries) */
typedef struct page_table {
    page_table_entry_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

/* Initialize paging */
void paging_init(void);

/* Enable paging */
void paging_enable(void);

/* Get the current page directory */
page_directory_t *paging_get_directory(void);

/* Create a new page directory */
page_directory_t *paging_create_directory(void);

/* Destroy a page directory */
void paging_destroy_directory(page_directory_t *dir);

/* Switch to a different page directory */
void paging_switch_directory(page_directory_t *dir);

/* Map a virtual address to a physical address */
void paging_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

/* Map a virtual address to a physical address in a specific directory */
void paging_map_page_dir(page_directory_t *dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

/* Unmap a virtual address */
void paging_unmap_page(uint32_t virt_addr);

/* Unmap a virtual address in a specific directory */
void paging_unmap_page_dir(page_directory_t *dir, uint32_t virt_addr);

/* Get physical address from virtual address */
uint32_t paging_get_physical_address(uint32_t virt_addr);

/* Get physical address from virtual address in a specific directory */
uint32_t paging_get_physical_address_dir(page_directory_t *dir, uint32_t virt_addr);

/* Page fault handler */
void page_fault_handler(void);

#endif /* PAGING_H */
