/* vfs.c - Virtual File System implementation */

#include "../include/vfs.h"
#include "../include/ext2.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/panic.h"

/* Global VFS state */
static vfs_state_t vfs_state;

/* Convert EXT2 file type to VFS file type */
static vfs_file_type_t ext2_to_vfs_type(uint16_t mode) {
    switch (mode & 0xF000) {
        case EXT2_S_IFREG:  return VFS_FILE_TYPE_REGULAR;
        case EXT2_S_IFDIR:  return VFS_FILE_TYPE_DIRECTORY;
        case EXT2_S_IFCHR:  return VFS_FILE_TYPE_CHARDEV;
        case EXT2_S_IFBLK:  return VFS_FILE_TYPE_BLOCKDEV;
        case EXT2_S_IFIFO:  return VFS_FILE_TYPE_FIFO;
        case EXT2_S_IFSOCK: return VFS_FILE_TYPE_SOCKET;
        case EXT2_S_IFLNK:  return VFS_FILE_TYPE_SYMLINK;
        default:            return VFS_FILE_TYPE_UNKNOWN;
    }
}

/* EXT2-backed file operations */
static int vfs_ext2_read(vfs_node_t *node, uint32_t offset, uint32_t size, void *buffer) {
    if (!node || !node->fs) {
        return -1;
    }

    return ext2_read_inode_data(node->fs, &node->ext2_inode, offset, size, buffer);
}

static int vfs_ext2_write(vfs_node_t *node, uint32_t offset, uint32_t size, const void *buffer) {
    /* TODO: Implement write support */
    (void)node; (void)offset; (void)size; (void)buffer;
    return -1;  /* Read-only for now */
}

/* Directory entry callback for readdir */
typedef struct {
    uint32_t index;
    uint32_t current;
    ext2_dir_entry_t *result;
} readdir_ctx_t;

static void readdir_callback(ext2_dir_entry_t *entry, void *ctx) {
    readdir_ctx_t *rctx = (readdir_ctx_t *)ctx;
    if (rctx->current == rctx->index) {
        rctx->result = entry;
    }
    rctx->current++;
}

static vfs_node_t *vfs_ext2_readdir(vfs_node_t *node, uint32_t index) {
    if (!node || !node->fs) {
        return NULL;
    }

    if (node->type != VFS_FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    readdir_ctx_t ctx = { .index = index, .current = 0, .result = NULL };
    ext2_read_dir(node->fs, node->inode, readdir_callback, &ctx);

    if (ctx.result) {
        /* Create VFS node for this entry */
        char name[256];
        uint32_t name_len = (ctx.result->name_len < 255) ? ctx.result->name_len : 255;
        memcpy(name, ctx.result->name, name_len);
        name[name_len] = '\0';

        return vfs_create_node_from_ext2(node->fs, ctx.result->inode, name);
    }

    return NULL;
}

/* Directory entry callback for finddir */
typedef struct {
    const char *name;
    uint32_t inode;
    bool found;
} finddir_ctx_t;

static void finddir_callback(ext2_dir_entry_t *entry, void *ctx) {
    finddir_ctx_t *fctx = (finddir_ctx_t *)ctx;

    if (fctx->found) {
        return;
    }

    char name[256];
    uint32_t name_len = (entry->name_len < 255) ? entry->name_len : 255;
    memcpy(name, entry->name, name_len);
    name[name_len] = '\0';

    if (strcmp(name, fctx->name) == 0) {
        fctx->inode = entry->inode;
        fctx->found = true;
    }
}

static vfs_node_t *vfs_ext2_finddir(vfs_node_t *node, const char *name) {
    if (!node || !node->fs || !name) {
        return NULL;
    }

    if (node->type != VFS_FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    finddir_ctx_t ctx = { .name = name, .inode = 0, .found = false };
    ext2_read_dir(node->fs, node->inode, finddir_callback, &ctx);

    if (ctx.found) {
        return vfs_create_node_from_ext2(node->fs, ctx.inode, name);
    }

    return NULL;
}

/* Create VFS node from EXT2 inode */
vfs_node_t *vfs_create_node_from_ext2(ext2_filesystem_t *fs, uint32_t inode_num, const char *name) {
    ext2_inode_t ext2_inode;

    if (ext2_read_inode(fs, inode_num, &ext2_inode) != 0) {
        return NULL;
    }

    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));

    /* Copy name */
    strncpy(node->name, name, 255);
    node->name[255] = '\0';

    /* Copy inode data */
    node->inode = inode_num;
    node->size = ext2_inode.i_size;
    node->type = ext2_to_vfs_type(ext2_inode.i_mode);
    node->links = ext2_inode.i_links_count;
    node->uid = ext2_inode.i_uid;
    node->gid = ext2_inode.i_gid;
    node->mode = ext2_inode.i_mode & 0xFFF;  /* Lower 12 bits are permissions */

    /* Store filesystem and EXT2 inode */
    node->fs = fs;
    memcpy(&node->ext2_inode, &ext2_inode, sizeof(ext2_inode_t));

    /* Set up operations */
    node->read = vfs_ext2_read;
    node->write = vfs_ext2_write;

    if (node->type == VFS_FILE_TYPE_DIRECTORY) {
        node->readdir = vfs_ext2_readdir;
        node->finddir = vfs_ext2_finddir;
    }

    return node;
}

/* Resolve a path to a VFS node */
vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path || path[0] != '/') {
        return NULL;
    }

    /* Start at root */
    vfs_node_t *current = vfs_state.root;
    if (!current) {
        return NULL;
    }

    /* Handle root directory */
    if (strcmp(path, "/") == 0) {
        return current;
    }

    /* Parse path */
    char path_copy[256];
    strncpy(path_copy, path + 1, 255);  /* Skip leading / */
    path_copy[255] = '\0';

    char *token = path_copy;
    char *next = NULL;

    while (token) {
        /* Find next / */
        next = strchr(token, '/');
        if (next) {
            *next = '\0';
            next++;
        }

        /* Look up component */
        if (current->finddir) {
            vfs_node_t *child = current->finddir(current, token);
            if (!child) {
                return NULL;  /* Component not found */
            }
            current = child;
        } else {
            return NULL;  /* Not a directory */
        }

        token = next;
    }

    return current;
}

/* VFS file operations */
int vfs_open(vfs_node_t *node, uint32_t flags) {
    if (!node) {
        return -1;
    }

    if (node->open) {
        return node->open(node, flags);
    }

    return 0;  /* Default: no-op */
}

void vfs_close(vfs_node_t *node) {
    if (!node) {
        return;
    }

    if (node->close) {
        node->close(node);
    }
}

int vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, void *buffer) {
    if (!node || !buffer) {
        return -1;
    }

    if (node->read) {
        return node->read(node, offset, size, buffer);
    }

    return -1;  /* No read operation */
}

int vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const void *buffer) {
    if (!node || !buffer) {
        return -1;
    }

    if (node->write) {
        return node->write(node, offset, size, buffer);
    }

    return -1;  /* No write operation */
}

vfs_node_t *vfs_readdir(vfs_node_t *node, uint32_t index) {
    if (!node) {
        return NULL;
    }

    if (node->readdir) {
        return node->readdir(node, index);
    }

    return NULL;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name) {
    if (!node || !name) {
        return NULL;
    }

    if (node->finddir) {
        return node->finddir(node, name);
    }

    return NULL;
}

/* Mount a filesystem */
int vfs_mount(const char *path, ext2_filesystem_t *fs) {
    if (!path || !fs || vfs_state.mount_count >= 16) {
        return -1;
    }

    /* Create root node for the filesystem */
    vfs_node_t *root_node = vfs_create_node_from_ext2(fs, fs->root_inode, "/");
    if (!root_node) {
        return -1;
    }

    /* Add to mount table */
    vfs_mount_t *mount = &vfs_state.mounts[vfs_state.mount_count];
    strncpy(mount->path, path, 255);
    mount->path[255] = '\0';
    mount->node = root_node;
    mount->fs = fs;
    vfs_state.mount_count++;

    /* If this is the first mount at /, set it as VFS root */
    if (strcmp(path, "/") == 0 && !vfs_state.root) {
        vfs_state.root = root_node;
    }

    printk("[VFS] Mounted filesystem at %s\n", path);
    return 0;
}

/* Unmount a filesystem */
int vfs_unmount(const char *path) {
    if (!path) {
        return -1;
    }

    /* Find mount point */
    for (uint32_t i = 0; i < vfs_state.mount_count; i++) {
        if (strcmp(vfs_state.mounts[i].path, path) == 0) {
            /* Free the root node */
            kfree(vfs_state.mounts[i].node);

            /* Remove from table by shifting */
            for (uint32_t j = i; j < vfs_state.mount_count - 1; j++) {
                vfs_state.mounts[j] = vfs_state.mounts[j + 1];
            }

            vfs_state.mount_count--;
            printk("[VFS] Unmounted filesystem at %s\n", path);
            return 0;
        }
    }

    return -1;  /* Mount point not found */
}

/* Get root node */
vfs_node_t *vfs_get_root(void) {
    return vfs_state.root;
}

/* Get file type name */
const char *vfs_get_type_name(vfs_file_type_t type) {
    switch (type) {
        case VFS_FILE_TYPE_REGULAR:    return "file";
        case VFS_FILE_TYPE_DIRECTORY:  return "dir";
        case VFS_FILE_TYPE_CHARDEV:    return "char";
        case VFS_FILE_TYPE_BLOCKDEV:   return "block";
        case VFS_FILE_TYPE_FIFO:       return "fifo";
        case VFS_FILE_TYPE_SOCKET:     return "socket";
        case VFS_FILE_TYPE_SYMLINK:    return "symlink";
        default:                       return "unknown";
    }
}

/* Print VFS tree (recursive) */
void vfs_print_tree(vfs_node_t *node, int depth) {
    if (!node) {
        return;
    }

    /* Print indentation */
    for (int i = 0; i < depth; i++) {
        printk("  ");
    }

    /* Print node info */
    printk("%s (%s, %d bytes, inode %d)\n",
           node->name,
           vfs_get_type_name(node->type),
           node->size,
           node->inode);

    /* Recurse for children */
    if (node->type == VFS_FILE_TYPE_DIRECTORY && node->readdir) {
        uint32_t i = 0;
        vfs_node_t *child;
        while ((child = node->readdir(node, i++)) != NULL) {
            /* Skip . and .. */
            if (strcmp(child->name, ".") == 0 || strcmp(child->name, "..") == 0) {
                kfree(child);
                continue;
            }
            vfs_print_tree(child, depth + 1);
            kfree(child);
        }
    }
}

/* Create a virtual directory node in memory */
static vfs_node_t *vfs_create_virtual_dir(const char *name) {
    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));
    strncpy(node->name, name, 255);
    node->name[255] = '\0';
    node->type = VFS_FILE_TYPE_DIRECTORY;
    node->size = 0;
    node->inode = 0;
    node->uid = 0;
    node->gid = 0;
    node->mode = 0755;

    return node;
}

/* Virtual file content storage */
typedef struct virtual_file_data {
    char *content;
    uint32_t size;
} virtual_file_data_t;

/* Read from virtual file */
static int vfs_virtual_read(vfs_node_t *node, uint32_t offset, uint32_t size, void *buffer) {
    if (!node || !buffer) {
        return -1;
    }

    /* Get file data from the inode field (we reuse it as a pointer) */
    virtual_file_data_t *data = (virtual_file_data_t *)(uintptr_t)node->inode;
    if (!data || !data->content) {
        return 0;
    }

    if (offset >= data->size) {
        return 0;
    }

    uint32_t to_read = size;
    if (offset + to_read > data->size) {
        to_read = data->size - offset;
    }

    memcpy(buffer, data->content + offset, to_read);
    return to_read;
}

/* Create a virtual file node in memory */
static vfs_node_t *vfs_create_virtual_file(const char *name, const char *content) {
    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));
    strncpy(node->name, name, 255);
    node->name[255] = '\0';
    node->type = VFS_FILE_TYPE_REGULAR;
    node->uid = 0;
    node->gid = 0;
    node->mode = 0644;

    /* Store file content */
    if (content) {
        virtual_file_data_t *data = kmalloc(sizeof(virtual_file_data_t));
        if (data) {
            data->size = strlen(content);
            data->content = kmalloc(data->size + 1);
            if (data->content) {
                strcpy(data->content, content);
                node->size = data->size;
                node->inode = (uint32_t)(uintptr_t)data;  /* Store pointer in inode field */
                node->read = vfs_virtual_read;
            } else {
                kfree(data);
            }
        }
    } else {
        node->size = 0;
    }

    return node;
}

/* Create basic directory structure for testing */
void vfs_create_base_dirs(void) {
    /* Create root directory if it doesn't exist */
    if (!vfs_state.root) {
        vfs_state.root = vfs_create_virtual_dir("/");
        printk("[VFS] Created virtual root directory\n");
    }

    /* Create /dev directory */
    vfs_node_t *dev = vfs_create_virtual_dir("dev");
    if (dev && vfs_state.root) {
        dev->father = vfs_state.root;
        dev->next_sibling = vfs_state.root->children;
        vfs_state.root->children = dev;
        printk("[VFS] Created /dev directory\n");
    }

    /* Create /tmp directory */
    vfs_node_t *tmp = vfs_create_virtual_dir("tmp");
    if (tmp && vfs_state.root) {
        tmp->father = vfs_state.root;
        tmp->next_sibling = vfs_state.root->children;
        vfs_state.root->children = tmp;
        printk("[VFS] Created /tmp directory\n");
    }

    /* Create /home directory */
    vfs_node_t *home = vfs_create_virtual_dir("home");
    if (home && vfs_state.root) {
        home->father = vfs_state.root;
        home->next_sibling = vfs_state.root->children;
        vfs_state.root->children = home;
        printk("[VFS] Created /home directory\n");
    }

    /* Create a test file in root */
    vfs_node_t *test = vfs_create_virtual_file("readme.txt", "Welcome to KFS-6!\nThis is a test file.\n");
    if (test && vfs_state.root) {
        test->father = vfs_state.root;
        test->next_sibling = vfs_state.root->children;
        vfs_state.root->children = test;
        printk("[VFS] Created /readme.txt test file\n");
    }
}

/* Simple readdir for virtual directories */
static vfs_node_t *vfs_virtual_readdir(vfs_node_t *node, uint32_t index) {
    if (!node || node->type != VFS_FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    uint32_t current = 0;
    vfs_node_t *child = node->children;

    while (child) {
        if (current == index) {
            /* Return a copy of the child node */
            vfs_node_t *copy = kmalloc(sizeof(vfs_node_t));
            if (copy) {
                memcpy(copy, child, sizeof(vfs_node_t));
            }
            return copy;
        }
        current++;
        child = child->next_sibling;
    }

    return NULL;
}

/* Simple finddir for virtual directories */
static vfs_node_t *vfs_virtual_finddir(vfs_node_t *node, const char *name) {
    if (!node || !name || node->type != VFS_FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    vfs_node_t *child = node->children;

    while (child) {
        if (strcmp(child->name, name) == 0) {
            /* Return the actual child node (not a copy) for path resolution */
            return child;
        }
        child = child->next_sibling;
    }

    return NULL;
}

/* Initialize VFS */
void vfs_init(void) {
    printk("[VFS] Initializing Virtual File System...\n");

    memset(&vfs_state, 0, sizeof(vfs_state_t));

    /* Create virtual root and base directories */
    vfs_create_base_dirs();

    /* Set up virtual directory operations */
    if (vfs_state.root) {
        vfs_state.root->readdir = vfs_virtual_readdir;
        vfs_state.root->finddir = vfs_virtual_finddir;

        /* Set up operations for all children */
        vfs_node_t *child = vfs_state.root->children;
        while (child) {
            if (child->type == VFS_FILE_TYPE_DIRECTORY) {
                child->readdir = vfs_virtual_readdir;
                child->finddir = vfs_virtual_finddir;
            }
            child = child->next_sibling;
        }
    }

    printk("[VFS] VFS initialized with virtual filesystem\n");
}
