#ifndef FOCUS_MEMORY_H
#define FOCUS_MEMORY_H

#include <stdint.h>

#include "focus/memory_reg.h"
#include "focus/status.h"

typedef struct memory_if {
    void *driver;
    status_t (*init)(void *);
    status_t (*read)(void *, const uint32_t, void *, const uint32_t);
    status_t (*write)(void *, const uint32_t, const void *, const uint32_t);
    status_t (*flush)(void *);

    uint32_t offset;
    struct memory_if *next;
    memory_reg_t **regs;
    uint32_t regs_num;
} memory_if_t;

status_t memory_init(memory_if_t *mem, memory_reg_t **regs, const uint32_t regs_size);
status_t memory_read(memory_if_t *mem, const uint32_t addr, void *data, const uint32_t len);
status_t memory_write(memory_if_t *mem, const uint32_t addr, const void *data, const uint32_t len);
status_t memory_flush(memory_if_t *mem);

#endif
