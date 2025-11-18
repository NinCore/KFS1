/* ide.h - IDE (ATA) disk driver for KFS-6 */

#ifndef IDE_H
#define IDE_H

#include "types.h"

/* IDE I/O Ports */
#define IDE_PRIMARY_IO_BASE     0x1F0
#define IDE_PRIMARY_CTRL_BASE   0x3F6
#define IDE_SECONDARY_IO_BASE   0x170
#define IDE_SECONDARY_CTRL_BASE 0x376

/* IDE Registers (offset from IO base) */
#define IDE_REG_DATA       0x00  /* Data register (16-bit) */
#define IDE_REG_ERROR      0x01  /* Error register (read) */
#define IDE_REG_FEATURES   0x01  /* Features register (write) */
#define IDE_REG_SECCOUNT   0x02  /* Sector count */
#define IDE_REG_LBALO      0x03  /* LBA low */
#define IDE_REG_LBAMID     0x04  /* LBA mid */
#define IDE_REG_LBAHI      0x05  /* LBA high */
#define IDE_REG_DRIVE      0x06  /* Drive/Head */
#define IDE_REG_STATUS     0x07  /* Status register (read) */
#define IDE_REG_COMMAND    0x07  /* Command register (write) */

/* IDE Control Register (offset from control base) */
#define IDE_REG_CONTROL    0x00  /* Control register */
#define IDE_REG_ALTSTATUS  0x00  /* Alternate status (read) */

/* IDE Status Register Flags */
#define IDE_STATUS_ERR   0x01  /* Error */
#define IDE_STATUS_DRQ   0x08  /* Data Request */
#define IDE_STATUS_SRV   0x10  /* Overlapped Mode Service Request */
#define IDE_STATUS_DF    0x20  /* Drive Fault Error */
#define IDE_STATUS_RDY   0x40  /* Drive Ready */
#define IDE_STATUS_BSY   0x80  /* Busy */

/* IDE Commands */
#define IDE_CMD_READ_SECTORS   0x20  /* Read sectors with retry */
#define IDE_CMD_WRITE_SECTORS  0x30  /* Write sectors with retry */
#define IDE_CMD_IDENTIFY       0xEC  /* Identify drive */
#define IDE_CMD_FLUSH          0xE7  /* Flush cache */

/* IDE Drive Selection */
#define IDE_DRIVE_MASTER  0xE0  /* Master drive (0xE0 = LBA mode, master) */
#define IDE_DRIVE_SLAVE   0xF0  /* Slave drive (0xF0 = LBA mode, slave) */

/* IDE Channel */
typedef enum {
    IDE_CHANNEL_PRIMARY = 0,
    IDE_CHANNEL_SECONDARY = 1
} ide_channel_t;

/* IDE Drive */
typedef enum {
    IDE_DRIVE_MASTER_IDX = 0,
    IDE_DRIVE_SLAVE_IDX = 1
} ide_drive_t;

/* IDE Device Information */
typedef struct {
    bool exists;              /* Device exists */
    uint16_t io_base;         /* I/O base address */
    uint16_t ctrl_base;       /* Control base address */
    uint8_t drive;            /* Drive select byte */
    uint32_t sectors;         /* Total sectors */
    uint32_t size_mb;         /* Size in MB */
    char model[41];           /* Model string */
    bool lba_supported;       /* LBA mode supported */
} ide_device_t;

/* IDE Controller */
typedef struct {
    ide_device_t devices[4];  /* 2 channels × 2 drives = 4 devices max */
} ide_controller_t;

/* IDE Functions */

/* Initialize IDE controller */
void ide_init(void);

/* Identify IDE device */
int ide_identify(ide_channel_t channel, ide_drive_t drive);

/* Read sectors from IDE device */
int ide_read_sectors(ide_channel_t channel, ide_drive_t drive,
                     uint32_t lba, uint8_t count, void *buffer);

/* Write sectors to IDE device */
int ide_write_sectors(ide_channel_t channel, ide_drive_t drive,
                      uint32_t lba, uint8_t count, const void *buffer);

/* Get IDE device info */
ide_device_t *ide_get_device(ide_channel_t channel, ide_drive_t drive);

/* Helper functions */
int ide_wait_ready(uint16_t io_base);
int ide_wait_drq(uint16_t io_base);
void ide_select_drive(uint16_t io_base, uint8_t drive);
uint8_t ide_read_status(uint16_t io_base);

/* Print IDE device information */
void ide_print_devices(void);

#endif /* IDE_H */
