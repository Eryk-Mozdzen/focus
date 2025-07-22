#ifndef IF_MEMORY_H
#define IF_MEMORY_H

#include <stdint.h>

#include "system/status.h"

typedef struct if_memory {
    void *driver;
    status_t (*init)(void *);
    status_t (*read)(void *, const uint32_t, void *, const uint32_t);
    status_t (*write)(void *, const uint32_t, const void *, const uint32_t);
    status_t (*flush)(void *);
} if_memory_t;

status_t memory_init(if_memory_t *mem);
status_t memory_read(if_memory_t *mem, const uint32_t addr, void *data, const uint32_t len);
status_t memory_write(if_memory_t *mem, const uint32_t addr, const void *data, const uint32_t len);
status_t memory_flush(if_memory_t *mem);

#endif
