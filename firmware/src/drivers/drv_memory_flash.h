#ifndef DRV_MEMORY_FLASH_H
#define DRV_MEMORY_FLASH_H

#include "system/if_memory.h"

typedef struct {
    if_memory_t interface;
} drv_memory_flash_t;

status_t drv_memory_flash_init(void *driver);
status_t drv_memory_flash_read(void *driver, const uint32_t addr, void *data, const uint32_t len);
status_t drv_memory_flash_write(void *driver, const uint32_t addr, const void *data, const uint32_t len);
status_t drv_memory_flash_flush(void *driver);

#endif
