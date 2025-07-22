#include "drivers/drv_memory_flash.h"

status_t drv_memory_flash_init(void *driver) {
    (void)driver;
    return STATUS_OK;
}

status_t drv_memory_flash_read(void *driver, const uint32_t addr, void *data, const uint32_t len) {
    (void)driver;
    (void)addr;
    (void)data;
    (void)len;
    return STATUS_OK;
}

status_t
drv_memory_flash_write(void *driver, const uint32_t addr, const void *data, const uint32_t len) {
    (void)driver;
    (void)addr;
    (void)data;
    (void)len;
    return STATUS_OK;
}

status_t drv_memory_flash_flush(void *driver) {
    (void)driver;
    return STATUS_OK;
}
