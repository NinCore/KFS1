/* gdt.c - Global Descriptor Table implementation */

#include "../include/gdt.h"
#include "../include/printf.h"
#include "../include/string.h"

/* GDT entries array - Must be at address 0x00000800 as per subject */
static struct gdt_entry gdt_entries[GDT_ENTRIES] __attribute__((section(".gdt")));
static struct gdt_ptr gdt_pointer;

/* Set a GDT entry */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    /* Base address */
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    /* Segment limit */
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = ((limit >> 16) & 0x0F);

    /* Granularity and access */
    gdt_entries[num].granularity |= (gran & 0xF0);
    gdt_entries[num].access      = access;
}

/* Initialize the GDT */
void gdt_init(void) {
    /* Set up GDT pointer */
    gdt_pointer.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_pointer.base = (uint32_t)&gdt_entries;

    /* NULL descriptor (entry 0) - required */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Kernel Code Segment (entry 1) */
    /* Base = 0x0, Limit = 0xFFFFFFFF (4GB) */
    /* Access = Present | Ring 0 | Code | Executable | Readable */
    gdt_set_gate(1, 0, 0xFFFFFFFF,
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE | GDT_ACCESS_EXEC | GDT_ACCESS_RW,
                 GDT_GRAN_4K | GDT_GRAN_32BIT | GDT_GRAN_LIMIT_HIGH);

    /* Kernel Data Segment (entry 2) */
    /* Base = 0x0, Limit = 0xFFFFFFFF (4GB) */
    /* Access = Present | Ring 0 | Data | Writable */
    gdt_set_gate(2, 0, 0xFFFFFFFF,
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA | GDT_ACCESS_RW,
                 GDT_GRAN_4K | GDT_GRAN_32BIT | GDT_GRAN_LIMIT_HIGH);

    /* Kernel Stack Segment (entry 3) */
    /* Base = 0x0, Limit = 0xFFFFFFFF (4GB) */
    /* Access = Present | Ring 0 | Data | Writable */
    gdt_set_gate(3, 0, 0xFFFFFFFF,
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA | GDT_ACCESS_RW,
                 GDT_GRAN_4K | GDT_GRAN_32BIT | GDT_GRAN_LIMIT_HIGH);

    /* User Code Segment (entry 4) */
    /* Base = 0x0, Limit = 0xFFFFFFFF (4GB) */
    /* Access = Present | Ring 3 | Code | Executable | Readable */
    gdt_set_gate(4, 0, 0xFFFFFFFF,
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE | GDT_ACCESS_EXEC | GDT_ACCESS_RW,
                 GDT_GRAN_4K | GDT_GRAN_32BIT | GDT_GRAN_LIMIT_HIGH);

    /* User Data Segment (entry 5) */
    /* Base = 0x0, Limit = 0xFFFFFFFF (4GB) */
    /* Access = Present | Ring 3 | Data | Writable */
    gdt_set_gate(5, 0, 0xFFFFFFFF,
                 GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA | GDT_ACCESS_RW,
                 GDT_GRAN_4K | GDT_GRAN_32BIT | GDT_GRAN_LIMIT_HIGH);

    /* Load the GDT */
    gdt_flush((uint32_t)&gdt_pointer);
}

/* Print GDT information for debugging */
void gdt_print_info(void) {
    printk("\n=== Global Descriptor Table ===\n");
    printk("GDT Base Address: 0x%x\n", gdt_pointer.base);
    printk("GDT Limit: %d bytes\n", gdt_pointer.limit + 1);
    printk("Number of Entries: %d\n\n", GDT_ENTRIES);

    const char* segment_names[] = {
        "NULL Descriptor",
        "Kernel Code",
        "Kernel Data",
        "Kernel Stack",
        "User Code",
        "User Data"
    };

    for (int i = 0; i < GDT_ENTRIES; i++) {
        uint32_t base = gdt_entries[i].base_low |
                        (gdt_entries[i].base_middle << 16) |
                        (gdt_entries[i].base_high << 24);

        uint32_t limit = gdt_entries[i].limit_low |
                         ((gdt_entries[i].granularity & 0x0F) << 16);

        printk("Entry %d: %s\n", i, segment_names[i]);
        printk("  Base:  0x%x\n", base);
        printk("  Limit: 0x%x\n", limit);
        printk("  Access: 0x%x\n", gdt_entries[i].access);
        printk("  Gran:  0x%x\n\n", gdt_entries[i].granularity);
    }
}
