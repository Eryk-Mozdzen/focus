#include "focus.h"

focus_status_t focus_memory_reset(focus_memory_t *memory) {
    if(!memory->reset) {
        FOCUS_LOG("memory reset: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    const uint32_t ret = memory->reset(memory->user);

    if(ret) {
        FOCUS_LOG("memory reset: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t
focus_memory_read(focus_memory_t *memory, const uint32_t addr, void *data, const uint32_t len) {
    if(!memory->read) {
        FOCUS_LOG("memory read: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    if(!data) {
        FOCUS_LOG("memory read: null destination");
        return FOCUS_STATUS_INVALID_ARG;
    }

    const uint32_t ret = memory->read(addr, data, len, memory->user);

    if(ret) {
        FOCUS_LOG("memory read: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t focus_memory_write(focus_memory_t *memory,
                                  const uint32_t addr,
                                  const void *data,
                                  const uint32_t len) {
    if(!memory->write) {
        FOCUS_LOG("memory write: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    if(!data) {
        FOCUS_LOG("memory write: null source");
        return FOCUS_STATUS_INVALID_ARG;
    }

    const uint32_t ret = memory->write(addr, data, len, memory->user);

    if(ret) {
        FOCUS_LOG("memory write: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t focus_memory_flush(focus_memory_t *memory) {
    if(!memory->flush) {
        FOCUS_LOG("memory flush: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    const uint32_t ret = memory->flush(memory->user);

    if(ret) {
        FOCUS_LOG("memory flush: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}
