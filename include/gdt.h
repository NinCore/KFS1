/* gdt.h - Global Descriptor Table definitions and functions */

#ifndef GDT_H
#define GDT_H

#include "types.h"

/* GDT Entry Structure */
struct gdt_entry {
    uint16_t limit_low;      /* Lower 16 bits of the limit */
    uint16_t base_low;       /* Lower 16 bits of the base */
    uint8_t  base_middle;    /* Next 8 bits of the base */
    uint8_t  access;         /* Access flags */
    uint8_t  granularity;    /* Granularity and upper 4 bits of limit */
    uint8_t  base_high;      /* Last 8 bits of the base */
} __attribute__((packed));

/* GDT Pointer Structure */
struct gdt_ptr {
    uint16_t limit;          /* Size of GDT - 1 */
    uint32_t base;           /* Address of the first GDT entry */
} __attribute__((packed));

/* Number of GDT entries */
#define GDT_ENTRIES 6

/* Segment selectors (offset into GDT) */
#define KERNEL_CODE_SEGMENT  0x08  /* 1st descriptor */
#define KERNEL_DATA_SEGMENT  0x10  /* 2nd descriptor */
#define KERNEL_STACK_SEGMENT 0x18  /* 3rd descriptor */
#define USER_CODE_SEGMENT    0x20  /* 4th descriptor */
#define USER_DATA_SEGMENT    0x28  /* 5th descriptor */
#define USER_STACK_SEGMENT   0x30  /* 6th descriptor */

/* Access byte flags */
#define GDT_ACCESS_PRESENT   0x80  /* Segment is present */
#define GDT_ACCESS_RING0     0x00  /* Ring 0 (kernel) */
#define GDT_ACCESS_RING3     0x60  /* Ring 3 (user) */
#define GDT_ACCESS_CODE      0x18  /* Code segment */
#define GDT_ACCESS_DATA      0x10  /* Data segment */
#define GDT_ACCESS_RW        0x02  /* Readable/Writable */
#define GDT_ACCESS_EXEC      0x08  /* Executable */

/* Granularity byte flags */
#define GDT_GRAN_4K          0x80  /* 4KB granularity */
#define GDT_GRAN_32BIT       0x40  /* 32-bit protected mode */
#define GDT_GRAN_LIMIT_HIGH  0x0F  /* Mask for high 4 bits of limit */

/* Initialize the GDT */
void gdt_init(void);

/* External assembly function to load GDT */
extern void gdt_flush(uint32_t gdt_ptr);

/* Get GDT info for debugging */
void gdt_print_info(void);

#endif /* GDT_H */
