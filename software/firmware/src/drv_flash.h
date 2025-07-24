#ifndef DRV_FLASH_H
#define DRV_FLASH_H

#include "focus/memory.h"

typedef struct {
    memory_if_t interface;
} drv_flash_t;

status_t drv_flash_init(void *driver);
status_t drv_flash_read(void *driver, const uint32_t addr, void *data, const uint32_t len);
status_t drv_flash_write(void *driver, const uint32_t addr, const void *data, const uint32_t len);
status_t drv_flash_flush(void *driver);

#endif
