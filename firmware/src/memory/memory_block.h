#ifndef MEMORY_BLOCK_H
#define MEMORY_BLOCK_H

#include <stdint.h>

#include "system/status.h"

typedef void (*memory_update_t)(void *);

typedef struct memory_block {
    uint32_t address;
    uint32_t size;
    memory_update_t callback;
    struct if_memory *mem;
    struct memory_block *next;
} memory_block_t;

status_t memory_block_register(if_memory_t *mem, memory_block_t *block);
status_t memory_block_read_float(memory_block_t *block, const uint32_t offset, float *value);
status_t memory_block_read_u32(memory_block_t *block, const uint32_t offset, uint32_t *value);
status_t memory_block_write_float(memory_block_t *block, const uint32_t offset, const float value);
status_t memory_block_write_u32(memory_block_t *block, const uint32_t offset, const uint32_t value);

#endif
