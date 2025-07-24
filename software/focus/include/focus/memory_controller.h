#ifndef FOCUS_MEMORY_CONTROLLER_H
#define FOCUS_MEMORY_CONTROLLER_H

#include "focus/memory.h"

typedef struct {
    memory_if_t interface;
    memory_if_t *list;
} memory_controller_t;

status_t memory_controller_init(memory_controller_t *ctrl);
status_t memory_controller_register(memory_controller_t *ctrl,
                                    memory_if_t *mem,
                                    const uint32_t offset,
                                    memory_reg_t **regs,
                                    const uint32_t regs_size);

#endif
