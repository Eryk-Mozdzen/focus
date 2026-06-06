#ifndef MSGPACK_H
#define MSGPACK_H

#include <stdbool.h>
#include <stdint.h>

#include <cmp.h>

typedef struct {
    cmp_ctx_t cmp;
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t position;
    uint32_t size;
} msgpack_t;

void msgpack_create_from(msgpack_t *msgpack, const uint8_t *buffer, const uint32_t size);
void msgpack_create_empty(msgpack_t *msgpack, uint8_t *buffer, const uint32_t capacity);
void msgpack_create_copy(msgpack_t *msgpack, const msgpack_t *other);

void msgpack_write_map(msgpack_t *msgpack, const uint32_t size);
void msgpack_write_array(msgpack_t *msgpack, const uint32_t size);
void msgpack_write_bool(msgpack_t *msgpack, const bool value);
void msgpack_write_str(msgpack_t *msgpack, const char *value);
void msgpack_write_strn(msgpack_t *msgpack, const char *value, const uint32_t size);
void msgpack_write_uint32(msgpack_t *msgpack, const uint32_t value);
void msgpack_write_float32(msgpack_t *msgpack, const float value);

bool msgpack_read_map(msgpack_t *msgpack, uint32_t *size);
bool msgpack_read_array(msgpack_t *msgpack, uint32_t *size);
bool msgpack_read_bool(msgpack_t *msgpack, bool *value);
bool msgpack_read_str(msgpack_t *msgpack, char *value, const uint32_t capacity);
bool msgpack_read_uint32(msgpack_t *msgpack, uint32_t *value);
bool msgpack_read_float32(msgpack_t *msgpack, float *value);
bool msgpack_read_array_float32(msgpack_t *msgpack,
                                float *values,
                                const uint32_t capacity,
                                uint32_t *size);

#endif
