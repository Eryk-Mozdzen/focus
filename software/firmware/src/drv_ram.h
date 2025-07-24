#ifndef DRV_RAM_H
#define DRV_RAM_H

#include "focus/memory.h"

typedef struct {
    memory_if_t interface;
} drv_ram_t;

status_t drv_ram_init(void *driver);
status_t drv_ram_read(void *driver, const uint32_t addr, void *data, const uint32_t len);
status_t drv_ram_write(void *driver, const uint32_t addr, const void *data, const uint32_t len);
status_t drv_ram_flush(void *driver);

#endif
