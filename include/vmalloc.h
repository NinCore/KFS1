/* vmalloc.h - Virtual memory allocation */

#ifndef VMALLOC_H
#define VMALLOC_H

#include "types.h"

/* Initialize virtual memory allocator */
void vmalloc_init(void);

/* Allocate virtual memory */
void *vmalloc(size_t size);

/* Free virtual memory */
void vfree(void *ptr);

/* Get size of allocated virtual block */
size_t vsize(void *ptr);

/* Get virtual memory statistics */
void vmalloc_stats(void);

#endif /* VMALLOC_H */
