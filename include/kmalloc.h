/* kmalloc.h - Physical memory allocation (kernel heap) */

#ifndef KMALLOC_H
#define KMALLOC_H

#include "types.h"

/* Initialize kernel memory allocator */
void kmalloc_init(void);

/* Allocate physical memory */
void *kmalloc(size_t size);

/* Allocate aligned physical memory */
void *kmalloc_aligned(size_t size, uint32_t align);

/* Free physical memory */
void kfree(void *ptr);

/* Get size of allocated block */
size_t ksize(void *ptr);

/* Get kernel heap statistics */
void kmalloc_stats(void);

#endif /* KMALLOC_H */
