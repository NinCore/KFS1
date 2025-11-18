/* vfs.h - Virtual File System for KFS-6 */

#ifndef VFS_H
#define VFS_H

#include "types.h"
#include "ext2.h"

/* File types */
typedef enum {
    VFS_FILE_TYPE_UNKNOWN = 0,
    VFS_FILE_TYPE_REGULAR,
    VFS_FILE_TYPE_DIRECTORY,
    VFS_FILE_TYPE_CHARDEV,
    VFS_FILE_TYPE_BLOCKDEV,
    VFS_FILE_TYPE_FIFO,
    VFS_FILE_TYPE_SOCKET,
    VFS_FILE_TYPE_SYMLINK
} vfs_file_type_t;

/* File permissions */
#define VFS_PERM_READ   0x4
#define VFS_PERM_WRITE  0x2
#define VFS_PERM_EXEC   0x1

/* File node (inode) structure */
typedef struct vfs_node {
    /* File information */
    char name[256];            /* File name */
    uint32_t size;             /* File size in bytes */
    vfs_file_type_t type;      /* File type */
    uint32_t inode;            /* Inode number */
    uint32_t links;            /* Hard links count */

    /* Tree structure */
    struct vfs_node *master;   /* Filesystem root */
    struct vfs_node *father;   /* Parent directory */
    struct vfs_node *children; /* First child (for directories) */
    struct vfs_node *next_sibling; /* Next sibling */

    /* Permissions */
    uint32_t uid;              /* Owner user ID */
    uint32_t gid;              /* Owner group ID */
    uint16_t mode;             /* Permission bits */

    /* Filesystem-specific data */
    ext2_filesystem_t *fs;     /* Filesystem pointer */
    ext2_inode_t ext2_inode;   /* EXT2 inode (if applicable) */

    /* File operations */
    int (*open)(struct vfs_node *node, uint32_t flags);
    void (*close)(struct vfs_node *node);
    int (*read)(struct vfs_node *node, uint32_t offset, uint32_t size, void *buffer);
    int (*write)(struct vfs_node *node, uint32_t offset, uint32_t size, const void *buffer);
    struct vfs_node *(*readdir)(struct vfs_node *node, uint32_t index);
    struct vfs_node *(*finddir)(struct vfs_node *node, const char *name);
} vfs_node_t;

/* Mount point structure */
typedef struct {
    char path[256];            /* Mount path */
    vfs_node_t *node;          /* Mounted filesystem root */
    ext2_filesystem_t *fs;     /* Filesystem */
} vfs_mount_t;

/* VFS Global state */
typedef struct {
    vfs_node_t *root;          /* Root directory (/) */
    vfs_mount_t mounts[16];    /* Mount table */
    uint32_t mount_count;      /* Number of mounts */
} vfs_state_t;

/* VFS Functions */

/* Initialize VFS */
void vfs_init(void);

/* Mount a filesystem */
int vfs_mount(const char *path, ext2_filesystem_t *fs);

/* Unmount a filesystem */
int vfs_unmount(const char *path);

/* Create VFS node from EXT2 inode */
vfs_node_t *vfs_create_node_from_ext2(ext2_filesystem_t *fs, uint32_t inode_num, const char *name);

/* Resolve a path to a VFS node */
vfs_node_t *vfs_resolve_path(const char *path);

/* Open a file */
int vfs_open(vfs_node_t *node, uint32_t flags);

/* Close a file */
void vfs_close(vfs_node_t *node);

/* Read from a file */
int vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, void *buffer);

/* Write to a file */
int vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const void *buffer);

/* Read directory entry by index */
vfs_node_t *vfs_readdir(vfs_node_t *node, uint32_t index);

/* Find file in directory by name */
vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name);

/* Get root node */
vfs_node_t *vfs_get_root(void);

/* Create basic directory structure (/dev, /proc, /sys, /var) */
void vfs_create_base_dirs(void);

/* Helper functions */
void vfs_print_tree(vfs_node_t *node, int depth);
const char *vfs_get_type_name(vfs_file_type_t type);

#endif /* VFS_H */
