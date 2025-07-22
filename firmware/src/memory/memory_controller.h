#ifndef MEMORY_CONTROLLER_H
#define MEMORY_CONTROLLER_H

#include "system/if_memory.h"

typedef struct {
    if_memory_t interface;
} memory_controller_t;

status_t memory_controller_init(memory_controller_t *ctrl);
status_t memory_controller_register(memory_controller_t *ctrl, if_memory_t *mem);

#endif
