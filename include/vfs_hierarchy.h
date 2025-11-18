/* vfs_hierarchy.h - Unix filesystem hierarchy for KFS-7 */

#ifndef VFS_HIERARCHY_H
#define VFS_HIERARCHY_H

#include "types.h"

/* Virtual file types */
#define VFILE_TYPE_REGULAR  0x01
#define VFILE_TYPE_DIR      0x02
#define VFILE_TYPE_DEVICE   0x03
#define VFILE_TYPE_PROC     0x04

/* Maximum virtual files */
#define MAX_VFILES 256

/* Virtual file entry */
typedef struct vfile {
    char name[64];
    char full_path[256];
    uint8_t type;
    uint32_t size;
    uint32_t inode;
    struct vfile *parent;
    struct vfile *children;
    struct vfile *next_sibling;
    void *data;  /* For special files */
    int in_use;
} vfile_t;

/* Virtual filesystem context */
typedef struct vfs_ctx {
    vfile_t files[MAX_VFILES];
    vfile_t *root;
    int file_count;
} vfs_ctx_t;

/* Initialize virtual filesystem hierarchy */
void vfs_hierarchy_init(void);

/* Directory operations */
vfile_t *vfs_create_directory(const char *path);
vfile_t *vfs_find_file(const char *path);
int vfs_list_directory(const char *path, char *buf, size_t buf_size);

/* Device file operations */
int vfs_create_device(const char *path, uint32_t major, uint32_t minor);

/* Proc file operations */
int vfs_create_proc_entry(const char *name, void *data);
int vfs_read_proc(const char *path, char *buf, size_t buf_size);

/* File operations */
int vfs_read_file(const char *path, char *buf, size_t buf_size);
int vfs_write_file(const char *path, const char *buf, size_t size);

/* Helper functions */
void vfs_print_tree(void);
const char *vfs_get_file_type_name(uint8_t type);

#endif /* VFS_HIERARCHY_H */
