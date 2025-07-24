#include "drv_ram.h"

status_t drv_ram_init(void *driver) {
    (void)driver;
    return STATUS_OK;
}

status_t drv_ram_read(void *driver, const uint32_t addr, void *data, const uint32_t len) {
    (void)driver;
    (void)addr;
    (void)data;
    (void)len;
    return STATUS_OK;
}

status_t drv_ram_write(void *driver, const uint32_t addr, const void *data, const uint32_t len) {
    (void)driver;
    (void)addr;
    (void)data;
    (void)len;
    return STATUS_OK;
}

status_t drv_ram_flush(void *driver) {
    (void)driver;
    return STATUS_OK;
}
