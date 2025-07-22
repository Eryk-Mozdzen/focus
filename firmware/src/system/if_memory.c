#include "system/if_memory.h"

status_t memory_init(if_memory_t *mem) {
    if(!mem->init) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->init(mem->driver);
}

status_t memory_read(if_memory_t *mem, const uint32_t addr, void *data, const uint32_t len) {
    if(!mem->read) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->read(mem->driver, addr, data, len);
}

status_t memory_write(if_memory_t *mem, const uint32_t addr, const void *data, const uint32_t len) {
    if(!mem->write) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->write(mem->driver, addr, data, len);
}

status_t memory_flush(if_memory_t *mem) {
    if(!mem->flush) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return mem->flush(mem->driver);
}
