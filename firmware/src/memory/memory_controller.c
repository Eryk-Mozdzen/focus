#include "memory/memory_controller.h"

static status_t init(void *driver) {
    (void)driver;
    return STATUS_OK;
}

static status_t read(void *driver, const uint32_t addr, void *data, const uint32_t len) {
    (void)driver;
    (void)addr;
    (void)data;
    (void)len;
    return STATUS_OK;
}

static status_t write(void *driver, const uint32_t addr, const void *data, const uint32_t len) {
    (void)driver;
    (void)addr;
    (void)data;
    (void)len;
    return STATUS_OK;
}

static status_t flush(void *driver) {
    (void)driver;
    return STATUS_OK;
}

status_t memory_controller_init(memory_controller_t *ctrl) {
    ctrl->interface.driver = ctrl;
    ctrl->interface.init = init;
    ctrl->interface.read = read;
    ctrl->interface.write = write;
    ctrl->interface.flush = flush;
    return STATUS_OK;
}

status_t memory_controller_register(memory_controller_t *ctrl, if_memory_t *mem) {
    (void)ctrl;
    (void)mem;
    return STATUS_OK;
}
