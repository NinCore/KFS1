/* ext2.c - EXT2 Filesystem implementation */

#include "../include/ext2.h"
#include "../include/ide.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/panic.h"

/* Get block size from superblock */
uint32_t ext2_get_block_size(ext2_filesystem_t *fs) {
    return 1024 << fs->superblock.s_log_block_size;
}

/* Get block group containing an inode */
uint32_t ext2_get_inode_block_group(ext2_filesystem_t *fs, uint32_t inode_num) {
    return (inode_num - 1) / fs->inodes_per_group;
}

/* Get index of inode within its block group */
uint32_t ext2_get_inode_index(ext2_filesystem_t *fs, uint32_t inode_num) {
    return (inode_num - 1) % fs->inodes_per_group;
}

/* Read superblock from disk */
int ext2_read_superblock(ext2_filesystem_t *fs) {
    /* Read sectors containing the superblock */
    /* Superblock is at byte 1024, which is sector 2 (512 * 2 = 1024) */
    uint8_t buffer[1024];

    /* Read 2 sectors starting from sector 2 */
    if (ide_read_sectors(fs->channel, fs->drive, 2, 2, buffer) != 0) {
        return -1;
    }

    /* Copy superblock */
    memcpy(&fs->superblock, buffer, sizeof(ext2_superblock_t));

    /* Check magic number */
    if (fs->superblock.s_magic != EXT2_MAGIC) {
        printk("[EXT2] Invalid magic number: 0x%x\n", fs->superblock.s_magic);
        return -1;
    }

    /* Calculate cached values */
    fs->block_size = ext2_get_block_size(fs);
    fs->blocks_per_group = fs->superblock.s_blocks_per_group;
    fs->inodes_per_group = fs->superblock.s_inodes_per_group;
    fs->groups_count = (fs->superblock.s_blocks_count + fs->blocks_per_group - 1) /
                       fs->blocks_per_group;
    fs->root_inode = 2;  /* Root directory is always inode 2 */

    return 0;
}

/* Read block group descriptors */
int ext2_read_group_descriptors(ext2_filesystem_t *fs) {
    /* Group descriptors start right after superblock */
    uint32_t gd_block = (fs->superblock.s_first_data_block == 0) ? 2 : 1;

    /* Calculate size needed */
    uint32_t gd_size = fs->groups_count * sizeof(ext2_group_desc_t);
    uint32_t gd_blocks = (gd_size + fs->block_size - 1) / fs->block_size;

    /* Allocate memory for group descriptors */
    fs->group_descriptors = (ext2_group_desc_t *)kmalloc(gd_size);
    if (!fs->group_descriptors) {
        return -1;
    }

    /* Read group descriptor blocks */
    uint8_t *gd_buffer = (uint8_t *)fs->group_descriptors;
    for (uint32_t i = 0; i < gd_blocks; i++) {
        if (ext2_read_block(fs, gd_block + i, gd_buffer + (i * fs->block_size)) != 0) {
            kfree(fs->group_descriptors);
            fs->group_descriptors = NULL;
            return -1;
        }
    }

    return 0;
}

/* Read a block from disk */
int ext2_read_block(ext2_filesystem_t *fs, uint32_t block_num, void *buffer) {
    /* Calculate sector number */
    uint32_t sectors_per_block = fs->block_size / 512;
    uint32_t start_sector = block_num * sectors_per_block;

    /* Read sectors */
    return ide_read_sectors(fs->channel, fs->drive, start_sector, sectors_per_block, buffer);
}

/* Write a block to disk */
int ext2_write_block(ext2_filesystem_t *fs, uint32_t block_num, const void *buffer) {
    /* Calculate sector number */
    uint32_t sectors_per_block = fs->block_size / 512;
    uint32_t start_sector = block_num * sectors_per_block;

    /* Write sectors */
    return ide_write_sectors(fs->channel, fs->drive, start_sector, sectors_per_block, buffer);
}

/* Read an inode from disk */
int ext2_read_inode(ext2_filesystem_t *fs, uint32_t inode_num, ext2_inode_t *inode) {
    if (inode_num == 0) {
        return -1;  /* Inode 0 is invalid */
    }

    /* Get block group and index */
    uint32_t group = ext2_get_inode_block_group(fs, inode_num);
    uint32_t index = ext2_get_inode_index(fs, inode_num);

    if (group >= fs->groups_count) {
        return -1;
    }

    /* Get inode table block */
    uint32_t inode_table = fs->group_descriptors[group].bg_inode_table;

    /* Calculate inode location */
    uint32_t inode_size = (fs->superblock.s_rev_level == 0) ? 128 : fs->superblock.s_inode_size;
    uint32_t inodes_per_block = fs->block_size / inode_size;
    uint32_t block_num = inode_table + (index / inodes_per_block);
    uint32_t block_offset = (index % inodes_per_block) * inode_size;

    /* Read block containing the inode */
    uint8_t *block_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!block_buffer) {
        return -1;
    }

    if (ext2_read_block(fs, block_num, block_buffer) != 0) {
        kfree(block_buffer);
        return -1;
    }

    /* Copy inode */
    memcpy(inode, block_buffer + block_offset, sizeof(ext2_inode_t));

    kfree(block_buffer);
    return 0;
}

/* Write an inode to disk */
int ext2_write_inode(ext2_filesystem_t *fs, uint32_t inode_num, const ext2_inode_t *inode) {
    if (inode_num == 0) {
        return -1;
    }

    /* Get block group and index */
    uint32_t group = ext2_get_inode_block_group(fs, inode_num);
    uint32_t index = ext2_get_inode_index(fs, inode_num);

    if (group >= fs->groups_count) {
        return -1;
    }

    /* Get inode table block */
    uint32_t inode_table = fs->group_descriptors[group].bg_inode_table;

    /* Calculate inode location */
    uint32_t inode_size = (fs->superblock.s_rev_level == 0) ? 128 : fs->superblock.s_inode_size;
    uint32_t inodes_per_block = fs->block_size / inode_size;
    uint32_t block_num = inode_table + (index / inodes_per_block);
    uint32_t block_offset = (index % inodes_per_block) * inode_size;

    /* Read block containing the inode */
    uint8_t *block_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!block_buffer) {
        return -1;
    }

    if (ext2_read_block(fs, block_num, block_buffer) != 0) {
        kfree(block_buffer);
        return -1;
    }

    /* Update inode in block */
    memcpy(block_buffer + block_offset, inode, sizeof(ext2_inode_t));

    /* Write block back */
    int result = ext2_write_block(fs, block_num, block_buffer);

    kfree(block_buffer);
    return result;
}

/* Read data from an inode (handles direct, indirect, double indirect blocks) */
int ext2_read_inode_data(ext2_filesystem_t *fs, ext2_inode_t *inode,
                         uint32_t offset, uint32_t size, void *buffer) {
    uint32_t blocks_read = 0;
    uint32_t bytes_read = 0;
    uint32_t start_block = offset / fs->block_size;
    uint32_t end_block = (offset + size + fs->block_size - 1) / fs->block_size;

    uint8_t *block_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!block_buffer) {
        return -1;
    }

    for (uint32_t i = start_block; i < end_block && bytes_read < size; i++) {
        uint32_t block_num = 0;

        /* Direct blocks (0-11) */
        if (i < 12) {
            block_num = inode->i_block[i];
        }
        /* Indirect blocks (12+) */
        else if (i < 12 + (fs->block_size / 4)) {
            uint32_t indirect_block = inode->i_block[12];
            if (indirect_block) {
                uint32_t *indirect_data = (uint32_t *)kmalloc(fs->block_size);
                if (ext2_read_block(fs, indirect_block, indirect_data) == 0) {
                    block_num = indirect_data[i - 12];
                }
                kfree(indirect_data);
            }
        }
        /* Double indirect blocks would go here, but we'll keep it simple */

        if (block_num == 0) {
            /* Sparse block - fill with zeros */
            uint32_t to_copy = (size - bytes_read > fs->block_size) ?
                               fs->block_size : (size - bytes_read);
            memset((uint8_t *)buffer + bytes_read, 0, to_copy);
            bytes_read += to_copy;
        } else {
            /* Read block */
            if (ext2_read_block(fs, block_num, block_buffer) == 0) {
                uint32_t block_offset = (i == start_block) ? (offset % fs->block_size) : 0;
                uint32_t to_copy = fs->block_size - block_offset;
                if (to_copy > size - bytes_read) {
                    to_copy = size - bytes_read;
                }
                memcpy((uint8_t *)buffer + bytes_read, block_buffer + block_offset, to_copy);
                bytes_read += to_copy;
            }
        }
    }

    kfree(block_buffer);
    return bytes_read;
}

/* Lookup a file in a directory */
int ext2_lookup(ext2_filesystem_t *fs, uint32_t dir_inode_num, const char *name, uint32_t *result_inode) {
    ext2_inode_t dir_inode;

    if (ext2_read_inode(fs, dir_inode_num, &dir_inode) != 0) {
        return -1;
    }

    /* Check if it's a directory */
    if ((dir_inode.i_mode & 0xF000) != EXT2_S_IFDIR) {
        return -1;
    }

    /* Allocate buffer for directory data */
    uint8_t *dir_data = (uint8_t *)kmalloc(dir_inode.i_size);
    if (!dir_data) {
        return -1;
    }

    /* Read directory data */
    if (ext2_read_inode_data(fs, &dir_inode, 0, dir_inode.i_size, dir_data) < 0) {
        kfree(dir_data);
        return -1;
    }

    /* Iterate through directory entries */
    uint32_t offset = 0;
    while (offset < dir_inode.i_size) {
        ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(dir_data + offset);

        if (entry->inode != 0) {
            /* Compare names */
            if (entry->name_len == strlen(name) &&
                strncmp(entry->name, name, entry->name_len) == 0) {
                *result_inode = entry->inode;
                kfree(dir_data);
                return 0;
            }
        }

        offset += entry->rec_len;

        if (entry->rec_len == 0) {
            break;  /* Invalid entry */
        }
    }

    kfree(dir_data);
    return -1;  /* Not found */
}

/* Read directory entries with callback */
int ext2_read_dir(ext2_filesystem_t *fs, uint32_t dir_inode_num,
                  void (*callback)(ext2_dir_entry_t *entry, void *ctx), void *ctx) {
    ext2_inode_t dir_inode;

    if (ext2_read_inode(fs, dir_inode_num, &dir_inode) != 0) {
        return -1;
    }

    /* Check if it's a directory */
    if ((dir_inode.i_mode & 0xF000) != EXT2_S_IFDIR) {
        return -1;
    }

    /* Allocate buffer for directory data */
    uint8_t *dir_data = (uint8_t *)kmalloc(dir_inode.i_size);
    if (!dir_data) {
        return -1;
    }

    /* Read directory data */
    if (ext2_read_inode_data(fs, &dir_inode, 0, dir_inode.i_size, dir_data) < 0) {
        kfree(dir_data);
        return -1;
    }

    /* Iterate through directory entries */
    uint32_t offset = 0;
    while (offset < dir_inode.i_size) {
        ext2_dir_entry_t *entry = (ext2_dir_entry_t *)(dir_data + offset);

        if (entry->inode != 0) {
            callback(entry, ctx);
        }

        offset += entry->rec_len;

        if (entry->rec_len == 0) {
            break;
        }
    }

    kfree(dir_data);
    return 0;
}

/* Initialize EXT2 filesystem */
int ext2_init(ext2_filesystem_t *fs, ide_channel_t channel, ide_drive_t drive) {
    printk("[EXT2] Initializing EXT2 filesystem...\n");

    memset(fs, 0, sizeof(ext2_filesystem_t));
    fs->channel = channel;
    fs->drive = drive;

    /* Read superblock */
    if (ext2_read_superblock(fs) != 0) {
        printk("[EXT2] Failed to read superblock\n");
        return -1;
    }

    /* Read group descriptors */
    if (ext2_read_group_descriptors(fs) != 0) {
        printk("[EXT2] Failed to read group descriptors\n");
        return -1;
    }

    ext2_print_info(fs);
    return 0;
}

/* Print filesystem information */
void ext2_print_info(ext2_filesystem_t *fs) {
    printk("\n=== EXT2 Filesystem Info ===\n");
    printk("Volume name: %s\n", fs->superblock.s_volume_name);
    printk("Block size: %d bytes\n", fs->block_size);
    printk("Total blocks: %d\n", fs->superblock.s_blocks_count);
    printk("Free blocks: %d\n", fs->superblock.s_free_blocks_count);
    printk("Total inodes: %d\n", fs->superblock.s_inodes_count);
    printk("Free inodes: %d\n", fs->superblock.s_free_inodes_count);
    printk("Blocks per group: %d\n", fs->blocks_per_group);
    printk("Inodes per group: %d\n", fs->inodes_per_group);
    printk("Block groups: %d\n", fs->groups_count);
    printk("Root inode: %d\n", fs->root_inode);
    printk("[EXT2] Filesystem initialized\n");
}
