/* vfs_hierarchy.c - Unix filesystem hierarchy implementation */

#include "../include/vfs_hierarchy.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/process.h"

/* Virtual filesystem context */
static vfs_ctx_t vfs_ctx;

/* Find free vfile slot */
static vfile_t *vfs_alloc_file(void) {
    for (int i = 0; i < MAX_VFILES; i++) {
        if (!vfs_ctx.files[i].in_use) {
            memset(&vfs_ctx.files[i], 0, sizeof(vfile_t));
            vfs_ctx.files[i].in_use = 1;
            vfs_ctx.files[i].inode = i + 1;
            vfs_ctx.file_count++;
            return &vfs_ctx.files[i];
        }
    }
    return NULL;
}

/* Split path into components */
static int vfs_split_path(const char *path, char components[32][64], int *count) {
    if (!path || path[0] != '/') {
        return -1;
    }

    *count = 0;
    const char *start = path + 1;  /* Skip leading / */
    const char *end;

    while (*start) {
        /* Find next / or end of string */
        end = start;
        while (*end && *end != '/') {
            end++;
        }

        /* Copy component */
        int len = end - start;
        if (len > 0 && len < 63) {
            strncpy(components[*count], start, len);
            components[*count][len] = '\0';
            (*count)++;
        }

        /* Move to next component */
        start = (*end == '/') ? end + 1 : end;

        if (*count >= 32) {
            return -1;  /* Too many components */
        }
    }

    return 0;
}

/* Find file by path */
vfile_t *vfs_find_file(const char *path) {
    if (!path) {
        return NULL;
    }

    /* Handle root directory */
    if (strcmp(path, "/") == 0) {
        return vfs_ctx.root;
    }

    /* Split path into components */
    char components[32][64];
    int count;
    if (vfs_split_path(path, components, &count) < 0) {
        return NULL;
    }

    /* Navigate from root */
    vfile_t *current = vfs_ctx.root;
    for (int i = 0; i < count; i++) {
        /* Search children */
        vfile_t *child = current->children;
        int found = 0;

        while (child) {
            if (strcmp(child->name, components[i]) == 0) {
                current = child;
                found = 1;
                break;
            }
            child = child->next_sibling;
        }

        if (!found) {
            return NULL;  /* Path component not found */
        }
    }

    return current;
}

/* Create directory */
vfile_t *vfs_create_directory(const char *path) {
    if (!path) {
        return NULL;
    }

    /* Check if already exists */
    if (vfs_find_file(path)) {
        return NULL;  /* Already exists */
    }

    /* Split path to get parent and name */
    char components[32][64];
    int count;
    if (vfs_split_path(path, components, &count) < 0 || count == 0) {
        return NULL;
    }

    /* Find parent directory */
    vfile_t *parent = vfs_ctx.root;
    for (int i = 0; i < count - 1; i++) {
        vfile_t *child = parent->children;
        int found = 0;

        while (child) {
            if (strcmp(child->name, components[i]) == 0) {
                parent = child;
                found = 1;
                break;
            }
            child = child->next_sibling;
        }

        if (!found) {
            return NULL;  /* Parent not found */
        }
    }

    /* Create new directory */
    vfile_t *dir = vfs_alloc_file();
    if (!dir) {
        return NULL;
    }

    strncpy(dir->name, components[count - 1], sizeof(dir->name) - 1);
    strncpy(dir->full_path, path, sizeof(dir->full_path) - 1);
    dir->type = VFILE_TYPE_DIR;
    dir->parent = parent;

    /* Add to parent's children */
    dir->next_sibling = parent->children;
    parent->children = dir;

    return dir;
}

/* Create device file */
int vfs_create_device(const char *path, uint32_t major, uint32_t minor) {
    if (!path) {
        return -1;
    }

    /* Check if already exists */
    if (vfs_find_file(path)) {
        return -1;
    }

    /* Split path */
    char components[32][64];
    int count;
    if (vfs_split_path(path, components, &count) < 0 || count == 0) {
        return -1;
    }

    /* Find parent directory */
    vfile_t *parent = vfs_ctx.root;
    for (int i = 0; i < count - 1; i++) {
        vfile_t *child = parent->children;
        int found = 0;

        while (child) {
            if (strcmp(child->name, components[i]) == 0) {
                parent = child;
                found = 1;
                break;
            }
            child = child->next_sibling;
        }

        if (!found) {
            return -1;
        }
    }

    /* Create device file */
    vfile_t *dev = vfs_alloc_file();
    if (!dev) {
        return -1;
    }

    strncpy(dev->name, components[count - 1], sizeof(dev->name) - 1);
    strncpy(dev->full_path, path, sizeof(dev->full_path) - 1);
    dev->type = VFILE_TYPE_DEVICE;
    dev->parent = parent;

    /* Store device numbers in data field */
    uint32_t *dev_nums = (uint32_t *)kmalloc(sizeof(uint32_t) * 2);
    if (dev_nums) {
        dev_nums[0] = major;
        dev_nums[1] = minor;
        dev->data = dev_nums;
    }

    /* Add to parent's children */
    dev->next_sibling = parent->children;
    parent->children = dev;

    return 0;
}

/* Create /proc entry */
int vfs_create_proc_entry(const char *name, void *data) {
    char path[256] = "/proc/";
    /* Manually concatenate strings (no snprintf in kernel) */
    int pos = 6;  /* Length of "/proc/" */
    while (*name && pos < 255) {
        path[pos++] = *name++;
    }
    path[pos] = '\0';

    vfile_t *file = vfs_alloc_file();
    if (!file) {
        return -1;
    }

    vfile_t *proc_dir = vfs_find_file("/proc");
    if (!proc_dir) {
        return -1;
    }

    strncpy(file->name, name, sizeof(file->name) - 1);
    strncpy(file->full_path, path, sizeof(file->full_path) - 1);
    file->type = VFILE_TYPE_PROC;
    file->parent = proc_dir;
    file->data = data;

    /* Add to proc's children */
    file->next_sibling = proc_dir->children;
    proc_dir->children = file;

    return 0;
}

/* List directory contents */
int vfs_list_directory(const char *path, char *buf, size_t buf_size) {
    vfile_t *dir = vfs_find_file(path);
    if (!dir || dir->type != VFILE_TYPE_DIR) {
        return -1;
    }

    size_t offset = 0;
    vfile_t *child = dir->children;

    while (child && offset < buf_size - 2) {
        /* Manually copy string (no snprintf in kernel) */
        const char *name = child->name;
        while (*name && offset < buf_size - 2) {
            buf[offset++] = *name++;
        }
        if (offset < buf_size - 1) {
            buf[offset++] = '\n';
        }
        child = child->next_sibling;
    }

    return (int)offset;
}

/* Get file type name */
const char *vfs_get_file_type_name(uint8_t type) {
    switch (type) {
        case VFILE_TYPE_REGULAR: return "file";
        case VFILE_TYPE_DIR: return "dir";
        case VFILE_TYPE_DEVICE: return "dev";
        case VFILE_TYPE_PROC: return "proc";
        default: return "unknown";
    }
}

/* Print filesystem tree */
static void vfs_print_tree_recursive(vfile_t *node, int depth) {
    if (!node) {
        return;
    }

    /* Print indentation */
    for (int i = 0; i < depth; i++) {
        printk("  ");
    }

    /* Print node */
    printk("%s [%s] (inode=%d)\n", node->name,
           vfs_get_file_type_name(node->type), node->inode);

    /* Print children */
    vfile_t *child = node->children;
    while (child) {
        vfs_print_tree_recursive(child, depth + 1);
        child = child->next_sibling;
    }
}

void vfs_print_tree(void) {
    printk("=== Virtual Filesystem Tree ===\n");
    if (vfs_ctx.root) {
        vfs_print_tree_recursive(vfs_ctx.root, 0);
    }
    printk("Total files: %d\n", vfs_ctx.file_count);
}

/* Initialize virtual filesystem hierarchy */
void vfs_hierarchy_init(void) {
    memset(&vfs_ctx, 0, sizeof(vfs_ctx_t));

    /* Create root directory */
    vfs_ctx.root = vfs_alloc_file();
    if (!vfs_ctx.root) {
        printk("[VFS] Failed to create root directory\n");
        return;
    }

    strncpy(vfs_ctx.root->name, "/", sizeof(vfs_ctx.root->name) - 1);
    strncpy(vfs_ctx.root->full_path, "/", sizeof(vfs_ctx.root->full_path) - 1);
    vfs_ctx.root->type = VFILE_TYPE_DIR;
    vfs_ctx.root->parent = NULL;

    /* Create standard directories */
    vfs_create_directory("/dev");
    vfs_create_directory("/proc");
    vfs_create_directory("/etc");
    vfs_create_directory("/bin");
    vfs_create_directory("/usr");
    vfs_create_directory("/usr/bin");
    vfs_create_directory("/home");
    vfs_create_directory("/root");
    vfs_create_directory("/tmp");

    /* Create device files in /dev */
    vfs_create_device("/dev/null", 1, 3);
    vfs_create_device("/dev/zero", 1, 5);
    vfs_create_device("/dev/console", 5, 1);
    vfs_create_device("/dev/tty", 5, 0);
    vfs_create_device("/dev/tty0", 4, 0);
    vfs_create_device("/dev/tty1", 4, 1);
    vfs_create_device("/dev/tty2", 4, 2);
    vfs_create_device("/dev/keyboard", 10, 1);
    vfs_create_device("/dev/mouse", 10, 2);

    /* Create /proc entries */
    vfs_create_proc_entry("version", NULL);
    vfs_create_proc_entry("cpuinfo", NULL);
    vfs_create_proc_entry("meminfo", NULL);
    vfs_create_proc_entry("uptime", NULL);

    printk("[VFS] Virtual filesystem hierarchy initialized\n");
    printk("[VFS] Created: /dev, /proc, /etc, /bin, /usr, /home, /root, /tmp\n");
}
