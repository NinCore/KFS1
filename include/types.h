/* types.h - Basic kernel types */

#ifndef TYPES_H
#define TYPES_H

/* Basic integer types */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

/* Size type */
typedef unsigned int size_t;

/* Pointer-sized integer types */
typedef unsigned int uintptr_t;
typedef signed int intptr_t;

/* Boolean type - use _Bool from C99 to avoid C23 keyword conflict */
#define bool _Bool
#define false 0
#define true 1

/* NULL pointer */
#define NULL ((void*)0)

#endif /* TYPES_H */
