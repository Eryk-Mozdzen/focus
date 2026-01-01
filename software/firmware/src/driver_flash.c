#include <focus.h>

uint32_t driver_flash_reset(void *user) {
    (void)user;
    return 0;
}

uint32_t driver_flash_read(const uint32_t addr, void *data, const uint32_t len, void *user) {
    (void)addr;
    (void)data;
    (void)len;
    (void)user;
    return 0;
}

uint32_t driver_flash_write(const uint32_t addr, const void *data, const uint32_t len, void *user) {
    (void)addr;
    (void)data;
    (void)len;
    (void)user;
    return 0;
}

uint32_t driver_flash_flush(void *user) {
    (void)user;
    return 0;
}
