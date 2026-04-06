#ifndef KERNEL_DRIVER_H
#define KERNEL_DRIVER_H

#include <kernel/types.h>

/*
 * Driver framework — simple registration and lookup model.
 * Every driver registers itself with a name and type; the kernel
 * can find and initialise it at boot or on demand.
 */

/* Driver class/category */
typedef enum {
    DRIVER_CHAR   = 0,   /* Character device (serial, keyboard, tty) */
    DRIVER_BLOCK  = 1,   /* Block device (disk, SD card) */
    DRIVER_NET    = 2,   /* Network interface */
    DRIVER_DISPLAY= 3,   /* Framebuffer / display */
    DRIVER_INPUT  = 4,   /* Input (keyboard, mouse) */
    DRIVER_BUS    = 5,   /* Bus controller (PCI, USB, I2C, SPI) */
    DRIVER_MISC   = 6,
} driver_type_t;

/* Driver state */
typedef enum {
    DRIVER_UNLOADED = 0,
    DRIVER_LOADED   = 1,
    DRIVER_RUNNING  = 2,
    DRIVER_ERROR    = 3,
} driver_state_t;

#define DRIVER_NAME_LEN  64

typedef struct driver {
    char           name[DRIVER_NAME_LEN];
    driver_type_t  type;
    driver_state_t state;

    /* Lifecycle */
    int  (*probe)(struct driver *drv);    /* Detect/init hardware */
    void (*remove)(struct driver *drv);   /* Detach / power down */
    void (*suspend)(struct driver *drv);
    void (*resume)(struct driver *drv);

    /* Character device ops (for DRIVER_CHAR) */
    ssize_t (*read)(struct driver *drv, void *buf, size_t count);
    ssize_t (*write)(struct driver *drv, const void *buf, size_t count);
    int     (*ioctl)(struct driver *drv, uint32_t cmd, void *arg);

    /* Block device ops (for DRIVER_BLOCK) */
    int (*bread)(struct driver *drv, uint64_t lba, uint32_t count, void *buf);
    int (*bwrite)(struct driver *drv, uint64_t lba, uint32_t count, const void *buf);
    uint64_t (*bsize)(struct driver *drv);   /* Sector size */
    uint64_t (*bcapacity)(struct driver *drv); /* Total sectors */

    void  *private_data;   /* Driver-private data */
    struct driver *next;   /* Linked list */
} driver_t;

/* Driver registry API */
void      driver_register(driver_t *drv);
void      driver_unregister(driver_t *drv);
driver_t *driver_find(const char *name);
driver_t *driver_find_by_type(driver_type_t type, uint32_t index);
void      drivers_probe_all(void);
void      drivers_list(void);

#endif /* KERNEL_DRIVER_H */
