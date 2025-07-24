#include "focus/memory.h"

status_t memory_init(memory_if_t *mem, memory_reg_t **regs, const uint32_t regs_size) {
    if(!mem->init) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    mem->regs = regs;
    mem->regs_num = regs_size / sizeof(memory_reg_t *);

    return mem->init(mem->driver);
}

status_t memory_read(memory_if_t *mem, const uint32_t addr, void *data, const uint32_t len) {
    if(!mem->read) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->read(mem->driver, addr, data, len);
}

status_t memory_write(memory_if_t *mem, const uint32_t addr, const void *data, const uint32_t len) {
    if(!mem->write) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->write(mem->driver, addr, data, len);
}

status_t memory_flush(memory_if_t *mem) {
    if(!mem->flush) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->flush(mem->driver);
}
