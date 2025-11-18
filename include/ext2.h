/* ext2.h - EXT2 Filesystem structures and functions for KFS-6 */

#ifndef EXT2_H
#define EXT2_H

#include "types.h"
#include "ide.h"

/* EXT2 Magic Number */
#define EXT2_MAGIC 0xEF53

/* EXT2 Superblock location (1024 bytes from start) */
#define EXT2_SUPERBLOCK_OFFSET 1024

/* EXT2 Block sizes */
#define EXT2_MIN_BLOCK_SIZE 1024
#define EXT2_MAX_BLOCK_SIZE 4096

/* EXT2 File Types */
#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7

/* EXT2 Inode modes */
#define EXT2_S_IFSOCK  0xC000  /* Socket */
#define EXT2_S_IFLNK   0xA000  /* Symbolic link */
#define EXT2_S_IFREG   0x8000  /* Regular file */
#define EXT2_S_IFBLK   0x6000  /* Block device */
#define EXT2_S_IFDIR   0x4000  /* Directory */
#define EXT2_S_IFCHR   0x2000  /* Character device */
#define EXT2_S_IFIFO   0x1000  /* FIFO */

/* EXT2 Superblock structure */
typedef struct {
    uint32_t s_inodes_count;        /* Total number of inodes */
    uint32_t s_blocks_count;        /* Total number of blocks */
    uint32_t s_r_blocks_count;      /* Reserved blocks count */
    uint32_t s_free_blocks_count;   /* Free blocks count */
    uint32_t s_free_inodes_count;   /* Free inodes count */
    uint32_t s_first_data_block;    /* First data block */
    uint32_t s_log_block_size;      /* Block size (log2(block_size) - 10) */
    uint32_t s_log_frag_size;       /* Fragment size */
    uint32_t s_blocks_per_group;    /* Blocks per group */
    uint32_t s_frags_per_group;     /* Fragments per group */
    uint32_t s_inodes_per_group;    /* Inodes per group */
    uint32_t s_mtime;               /* Mount time */
    uint32_t s_wtime;               /* Write time */
    uint16_t s_mnt_count;           /* Mount count */
    uint16_t s_max_mnt_count;       /* Maximal mount count */
    uint16_t s_magic;               /* Magic signature (0xEF53) */
    uint16_t s_state;               /* File system state */
    uint16_t s_errors;              /* Behavior when detecting errors */
    uint16_t s_minor_rev_level;     /* Minor revision level */
    uint32_t s_lastcheck;           /* Time of last check */
    uint32_t s_checkinterval;       /* Max time between checks */
    uint32_t s_creator_os;          /* OS */
    uint32_t s_rev_level;           /* Revision level */
    uint16_t s_def_resuid;          /* Default uid for reserved blocks */
    uint16_t s_def_resgid;          /* Default gid for reserved blocks */
    /* Extended fields */
    uint32_t s_first_ino;           /* First non-reserved inode */
    uint16_t s_inode_size;          /* Size of inode structure */
    uint16_t s_block_group_nr;      /* Block group # of this superblock */
    uint32_t s_feature_compat;      /* Compatible feature set */
    uint32_t s_feature_incompat;    /* Incompatible feature set */
    uint32_t s_feature_ro_compat;   /* Readonly-compatible feature set */
    uint8_t  s_uuid[16];            /* 128-bit uuid for volume */
    char     s_volume_name[16];     /* Volume name */
    char     s_last_mounted[64];    /* Directory where last mounted */
    uint32_t s_algorithm_usage_bitmap; /* For compression */
    uint8_t  s_prealloc_blocks;     /* Nr of blocks to try to preallocate */
    uint8_t  s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    uint16_t s_padding1;
    uint8_t  s_reserved[204];       /* Padding to the end of the block */
} __attribute__((packed)) ext2_superblock_t;

/* EXT2 Block Group Descriptor */
typedef struct {
    uint32_t bg_block_bitmap;       /* Block bitmap block */
    uint32_t bg_inode_bitmap;       /* Inode bitmap block */
    uint32_t bg_inode_table;        /* Inode table block */
    uint16_t bg_free_blocks_count;  /* Free blocks count */
    uint16_t bg_free_inodes_count;  /* Free inodes count */
    uint16_t bg_used_dirs_count;    /* Directories count */
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
} __attribute__((packed)) ext2_group_desc_t;

/* EXT2 Inode structure */
typedef struct {
    uint16_t i_mode;        /* File mode */
    uint16_t i_uid;         /* Owner UID */
    uint32_t i_size;        /* Size in bytes */
    uint32_t i_atime;       /* Access time */
    uint32_t i_ctime;       /* Creation time */
    uint32_t i_mtime;       /* Modification time */
    uint32_t i_dtime;       /* Deletion time */
    uint16_t i_gid;         /* Group ID */
    uint16_t i_links_count; /* Links count */
    uint32_t i_blocks;      /* Blocks count (in 512-byte sectors) */
    uint32_t i_flags;       /* File flags */
    uint32_t i_osd1;        /* OS dependent 1 */
    uint32_t i_block[15];   /* Pointers to blocks */
    uint32_t i_generation;  /* File version (for NFS) */
    uint32_t i_file_acl;    /* File ACL */
    uint32_t i_dir_acl;     /* Directory ACL */
    uint32_t i_faddr;       /* Fragment address */
    uint8_t  i_osd2[12];    /* OS dependent 2 */
} __attribute__((packed)) ext2_inode_t;

/* EXT2 Directory Entry */
typedef struct {
    uint32_t inode;         /* Inode number */
    uint16_t rec_len;       /* Directory entry length */
    uint8_t  name_len;      /* Name length */
    uint8_t  file_type;     /* File type */
    char     name[];        /* File name (variable length) */
} __attribute__((packed)) ext2_dir_entry_t;

/* EXT2 Filesystem in-memory structure */
typedef struct {
    /* Disk device */
    ide_channel_t channel;
    ide_drive_t drive;

    /* Superblock */
    ext2_superblock_t superblock;

    /* Cached data */
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t groups_count;

    /* Block group descriptors */
    ext2_group_desc_t *group_descriptors;

    /* Root inode */
    uint32_t root_inode;
} ext2_filesystem_t;

/* EXT2 Functions */

/* Initialize EXT2 filesystem from IDE device */
int ext2_init(ext2_filesystem_t *fs, ide_channel_t channel, ide_drive_t drive);

/* Read superblock */
int ext2_read_superblock(ext2_filesystem_t *fs);

/* Read block group descriptors */
int ext2_read_group_descriptors(ext2_filesystem_t *fs);

/* Read a block from disk */
int ext2_read_block(ext2_filesystem_t *fs, uint32_t block_num, void *buffer);

/* Write a block to disk */
int ext2_write_block(ext2_filesystem_t *fs, uint32_t block_num, const void *buffer);

/* Read an inode */
int ext2_read_inode(ext2_filesystem_t *fs, uint32_t inode_num, ext2_inode_t *inode);

/* Write an inode */
int ext2_write_inode(ext2_filesystem_t *fs, uint32_t inode_num, const ext2_inode_t *inode);

/* Read data from an inode */
int ext2_read_inode_data(ext2_filesystem_t *fs, ext2_inode_t *inode,
                         uint32_t offset, uint32_t size, void *buffer);

/* Lookup a file in a directory */
int ext2_lookup(ext2_filesystem_t *fs, uint32_t dir_inode, const char *name, uint32_t *result_inode);

/* Read directory entries */
int ext2_read_dir(ext2_filesystem_t *fs, uint32_t dir_inode,
                  void (*callback)(ext2_dir_entry_t *entry, void *ctx), void *ctx);

/* Helper functions */
uint32_t ext2_get_block_size(ext2_filesystem_t *fs);
uint32_t ext2_get_inode_block_group(ext2_filesystem_t *fs, uint32_t inode_num);
uint32_t ext2_get_inode_index(ext2_filesystem_t *fs, uint32_t inode_num);

/* Print filesystem information */
void ext2_print_info(ext2_filesystem_t *fs);

#endif /* EXT2_H */
