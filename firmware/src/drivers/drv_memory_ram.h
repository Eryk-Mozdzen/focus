#ifndef DRV_MEMORY_RAM_H
#define DRV_MEMORY_RAM_H

#include "system/if_memory.h"

typedef struct {
    if_memory_t interface;
} drv_memory_ram_t;

status_t drv_memory_ram_init(void *driver);
status_t drv_memory_ram_read(void *driver, const uint32_t addr, void *data, const uint32_t len);
status_t drv_memory_ram_write(void *driver, const uint32_t addr, const void *data, const uint32_t len);
status_t drv_memory_ram_flush(void *driver);

#endif
