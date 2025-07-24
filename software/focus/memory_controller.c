#include <stddef.h>
#include <stdint.h>

#include "focus/memory_controller.h"

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
    ctrl->list = NULL;
    return STATUS_OK;
}

status_t memory_controller_register(memory_controller_t *ctrl,
                                    memory_if_t *mem,
                                    const uint32_t offset,
                                    memory_reg_t **regs,
                                    const uint32_t regs_size) {
    mem->offset = offset;
    mem->next = ctrl->list;
    ctrl->list = mem;
    return memory_init(mem, regs, regs_size);
}
