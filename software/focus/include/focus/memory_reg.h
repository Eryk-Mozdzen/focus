#ifndef FOCUS_MEMORY_REG_H
#define FOCUS_MEMORY_REG_H

#include <stdint.h>

#include "focus/memory.h"
#include "focus/status.h"

typedef enum {
    MEMORY_REG_ACCESS_R = 0x01,
    MEMORY_REG_ACCESS_W = 0x02,
    MEMORY_REG_ACCESS_RW = 0x03,
} memory_reg_access_t;

typedef union {
    uint8_t bytes[4];
    int32_t integer;
    float floating;
} memory_reg_value_t;

typedef void (*memory_reg_update_t)(void *, const memory_reg_value_t);

typedef struct {
    uint32_t offset;
    memory_if_t *mem;
    memory_reg_update_t callback;
    void *context;
} memory_reg_t;

status_t memory_reg_read_float(memory_reg_t *reg, float *value);
status_t memory_reg_read_int(memory_reg_t *reg, int32_t *value);
status_t memory_reg_write_float(memory_reg_t *reg, const float value);
status_t memory_reg_write_int(memory_reg_t *reg, const int32_t value);

#endif
