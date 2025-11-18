/* ide.c - IDE (ATA) disk driver implementation */

#include "../include/ide.h"
#include "../include/io.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/panic.h"

/* Global IDE controller */
static ide_controller_t ide_controller;

/* Wait for IDE device to be ready (not busy) */
int ide_wait_ready(uint16_t io_base) {
    uint8_t status;
    int timeout = 10000;

    while (timeout-- > 0) {
        status = inb(io_base + IDE_REG_STATUS);
        if (!(status & IDE_STATUS_BSY) && (status & IDE_STATUS_RDY)) {
            return 0;  /* Ready */
        }
    }

    return -1;  /* Timeout */
}

/* Wait for IDE device to have data ready (DRQ set) */
int ide_wait_drq(uint16_t io_base) {
    uint8_t status;
    int timeout = 10000;

    while (timeout-- > 0) {
        status = inb(io_base + IDE_REG_STATUS);
        if (status & IDE_STATUS_DRQ) {
            return 0;  /* Data ready */
        }
        if (status & IDE_STATUS_ERR) {
            return -1;  /* Error */
        }
    }

    return -1;  /* Timeout */
}

/* Select IDE drive */
void ide_select_drive(uint16_t io_base, uint8_t drive) {
    outb(io_base + IDE_REG_DRIVE, drive);
    /* Wait 400ns after drive select */
    for (int i = 0; i < 4; i++) {
        inb(io_base + IDE_REG_ALTSTATUS);
    }
}

/* Read IDE status register */
uint8_t ide_read_status(uint16_t io_base) {
    return inb(io_base + IDE_REG_STATUS);
}

/* Identify IDE device */
int ide_identify(ide_channel_t channel, ide_drive_t drive) {
    uint16_t io_base = (channel == IDE_CHANNEL_PRIMARY) ?
                       IDE_PRIMARY_IO_BASE : IDE_SECONDARY_IO_BASE;
    uint16_t ctrl_base = (channel == IDE_CHANNEL_PRIMARY) ?
                         IDE_PRIMARY_CTRL_BASE : IDE_SECONDARY_CTRL_BASE;
    uint8_t drive_select = (drive == IDE_DRIVE_MASTER_IDX) ?
                           IDE_DRIVE_MASTER : IDE_DRIVE_SLAVE;

    int dev_idx = (channel * 2) + drive;
    ide_device_t *device = &ide_controller.devices[dev_idx];

    /* Initialize device structure */
    device->io_base = io_base;
    device->ctrl_base = ctrl_base;
    device->drive = drive_select;
    device->exists = false;

    /* Select drive */
    ide_select_drive(io_base, drive_select);

    /* Send IDENTIFY command */
    outb(io_base + IDE_REG_COMMAND, IDE_CMD_IDENTIFY);

    /* Check if drive exists */
    uint8_t status = ide_read_status(io_base);
    if (status == 0) {
        /* No drive */
        return -1;
    }

    /* Wait for ready or error */
    if (ide_wait_ready(io_base) != 0) {
        return -1;
    }

    /* Wait for DRQ */
    if (ide_wait_drq(io_base) != 0) {
        return -1;
    }

    /* Read identification data (256 words = 512 bytes) */
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(io_base + IDE_REG_DATA);
    }

    /* Check if LBA is supported (bit 9 of word 49) */
    device->lba_supported = (identify_data[49] & (1 << 9)) != 0;

    if (!device->lba_supported) {
        printk("[IDE] Drive does not support LBA\n");
        return -1;
    }

    /* Get total sectors (words 60-61 for 28-bit LBA) */
    device->sectors = (identify_data[61] << 16) | identify_data[60];
    device->size_mb = (device->sectors * 512) / (1024 * 1024);

    /* Get model string (words 27-46, 40 bytes) */
    for (int i = 0; i < 20; i++) {
        device->model[i * 2] = identify_data[27 + i] >> 8;
        device->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
    }
    device->model[40] = '\0';

    /* Trim trailing spaces */
    for (int i = 39; i >= 0 && device->model[i] == ' '; i--) {
        device->model[i] = '\0';
    }

    device->exists = true;
    return 0;
}

/* Read sectors from IDE device */
int ide_read_sectors(ide_channel_t channel, ide_drive_t drive,
                     uint32_t lba, uint8_t count, void *buffer) {
    int dev_idx = (channel * 2) + drive;
    ide_device_t *device = &ide_controller.devices[dev_idx];

    if (!device->exists) {
        return -1;
    }

    if (count == 0) {
        return -1;
    }

    uint16_t io_base = device->io_base;
    uint8_t drive_select = device->drive;

    /* Wait for drive to be ready */
    if (ide_wait_ready(io_base) != 0) {
        return -1;
    }

    /* Select drive and set LBA */
    outb(io_base + IDE_REG_DRIVE, drive_select | ((lba >> 24) & 0x0F));

    /* Set sector count */
    outb(io_base + IDE_REG_SECCOUNT, count);

    /* Set LBA */
    outb(io_base + IDE_REG_LBALO, lba & 0xFF);
    outb(io_base + IDE_REG_LBAMID, (lba >> 8) & 0xFF);
    outb(io_base + IDE_REG_LBAHI, (lba >> 16) & 0xFF);

    /* Send READ SECTORS command */
    outb(io_base + IDE_REG_COMMAND, IDE_CMD_READ_SECTORS);

    /* Read sectors */
    uint16_t *buf = (uint16_t *)buffer;
    for (int sector = 0; sector < count; sector++) {
        /* Wait for DRQ */
        if (ide_wait_drq(io_base) != 0) {
            return -1;
        }

        /* Read 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            buf[sector * 256 + i] = inw(io_base + IDE_REG_DATA);
        }
    }

    return 0;
}

/* Write sectors to IDE device */
int ide_write_sectors(ide_channel_t channel, ide_drive_t drive,
                      uint32_t lba, uint8_t count, const void *buffer) {
    int dev_idx = (channel * 2) + drive;
    ide_device_t *device = &ide_controller.devices[dev_idx];

    if (!device->exists) {
        return -1;
    }

    if (count == 0) {
        return -1;
    }

    uint16_t io_base = device->io_base;
    uint8_t drive_select = device->drive;

    /* Wait for drive to be ready */
    if (ide_wait_ready(io_base) != 0) {
        return -1;
    }

    /* Select drive and set LBA */
    outb(io_base + IDE_REG_DRIVE, drive_select | ((lba >> 24) & 0x0F));

    /* Set sector count */
    outb(io_base + IDE_REG_SECCOUNT, count);

    /* Set LBA */
    outb(io_base + IDE_REG_LBALO, lba & 0xFF);
    outb(io_base + IDE_REG_LBAMID, (lba >> 8) & 0xFF);
    outb(io_base + IDE_REG_LBAHI, (lba >> 16) & 0xFF);

    /* Send WRITE SECTORS command */
    outb(io_base + IDE_REG_COMMAND, IDE_CMD_WRITE_SECTORS);

    /* Write sectors */
    const uint16_t *buf = (const uint16_t *)buffer;
    for (int sector = 0; sector < count; sector++) {
        /* Wait for DRQ */
        if (ide_wait_drq(io_base) != 0) {
            return -1;
        }

        /* Write 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            outw(io_base + IDE_REG_DATA, buf[sector * 256 + i]);
        }

        /* Wait for write to complete */
        if (ide_wait_ready(io_base) != 0) {
            return -1;
        }
    }

    /* Flush cache */
    outb(io_base + IDE_REG_COMMAND, IDE_CMD_FLUSH);
    ide_wait_ready(io_base);

    return 0;
}

/* Get IDE device info */
ide_device_t *ide_get_device(ide_channel_t channel, ide_drive_t drive) {
    int dev_idx = (channel * 2) + drive;
    return &ide_controller.devices[dev_idx];
}

/* Print IDE device information */
void ide_print_devices(void) {
    printk("\n=== IDE Devices ===\n");

    const char *channel_names[] = {"Primary", "Secondary"};
    const char *drive_names[] = {"Master", "Slave"};

    for (int channel = 0; channel < 2; channel++) {
        for (int drive = 0; drive < 2; drive++) {
            int dev_idx = (channel * 2) + drive;
            ide_device_t *device = &ide_controller.devices[dev_idx];

            printk("%s %s: ", channel_names[channel], drive_names[drive]);

            if (device->exists) {
                printk("%s (%d MB, %d sectors)\n",
                       device->model, device->size_mb, device->sectors);
            } else {
                printk("Not detected\n");
            }
        }
    }
}

/* Initialize IDE controller */
void ide_init(void) {
    printk("[IDE] Initializing IDE controller...\n");

    /* Clear device table */
    memset(&ide_controller, 0, sizeof(ide_controller_t));

    /* Detect devices on both channels */
    for (int channel = 0; channel < 2; channel++) {
        for (int drive = 0; drive < 2; drive++) {
            ide_identify(channel, drive);
        }
    }

    /* Print detected devices */
    ide_print_devices();

    printk("[IDE] IDE controller initialized\n");
}
