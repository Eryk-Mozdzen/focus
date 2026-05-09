#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <cmp.h>

#include "msgpack.h"

static size_t writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    msgpack_t *msgpack = (msgpack_t *)ctx->buf;

    if((msgpack->position + count) > msgpack->capacity) {
        return 0;
    }

    memcpy(msgpack->buffer + msgpack->position, data, count);
    msgpack->position += count;
    msgpack->size = msgpack->position;
    return count;
}

static bool reader(cmp_ctx_t *ctx, void *data, size_t count) {
    msgpack_t *msgpack = (msgpack_t *)ctx->buf;

    if((msgpack->position + count) > msgpack->size) {
        return false;
    }

    memcpy(data, msgpack->buffer + msgpack->position, count);
    msgpack->position += count;
    return true;
}

void msgpack_create_from(msgpack_t *msgpack, const uint8_t *buffer, const uint32_t size) {
    msgpack->buffer = (uint8_t *)buffer;
    msgpack->capacity = size;
    msgpack->size = size;
    msgpack->position = 0,

    cmp_init(&msgpack->cmp, msgpack, reader, NULL, writer);
}

void msgpack_create_empty(msgpack_t *msgpack, uint8_t *buffer, const uint32_t capacity) {
    msgpack->buffer = buffer;
    msgpack->capacity = capacity;
    msgpack->size = 0;
    msgpack->position = 0;

    cmp_init(&msgpack->cmp, msgpack, reader, NULL, writer);
}

void msgpack_create_copy(msgpack_t *msgpack, const msgpack_t *other) {
    *msgpack = *other;
    msgpack->cmp.buf = msgpack;
}

void msgpack_write_map(msgpack_t *msgpack, const uint32_t size) {
    cmp_write_map(&msgpack->cmp, size);
}

void msgpack_write_array(msgpack_t *msgpack, const uint32_t size) {
    cmp_write_array(&msgpack->cmp, size);
}

void msgpack_write_bool(msgpack_t *msgpack, const bool value) {
    cmp_write_bool(&msgpack->cmp, value);
}

void msgpack_write_str(msgpack_t *msgpack, const char *value) {
    cmp_write_str(&msgpack->cmp, value, strlen(value));
}

void msgpack_write_strn(msgpack_t *msgpack, const char *value, const uint32_t size) {
    cmp_write_str(&msgpack->cmp, value, size);
}

void msgpack_write_uint32(msgpack_t *msgpack, const uint32_t value) {
    cmp_write_u32(&msgpack->cmp, value);
}

void msgpack_write_float32(msgpack_t *msgpack, const float value) {
    cmp_write_float(&msgpack->cmp, value);
}

bool msgpack_read_map(msgpack_t *msgpack, uint32_t *size) {
    *size = 0;
    return cmp_read_map(&msgpack->cmp, size);
}

bool msgpack_read_array(msgpack_t *msgpack, uint32_t *size) {
    *size = 0;
    return cmp_read_array(&msgpack->cmp, size);
}

bool msgpack_read_bool(msgpack_t *msgpack, bool *value) {
    *value = false;
    return cmp_read_bool(&msgpack->cmp, value);
}

bool msgpack_read_str(msgpack_t *msgpack, char *value, const uint32_t capacity) {
    memset(value, '\0', capacity);

    uint32_t len = capacity;
    const bool result = cmp_read_str(&msgpack->cmp, value, &len);

    value[capacity - 1] = '\0';

    return result;
}

bool msgpack_read_uint32(msgpack_t *msgpack, uint32_t *value) {
    *value = 0;

    cmp_object_t object;
    if(!cmp_read_object(&msgpack->cmp, &object)) {
        return false;
    }

    int8_t int8;
    if(cmp_object_as_char(&object, &int8)) {
        *value = int8;
        return true;
    }

    int16_t int16;
    if(cmp_object_as_short(&object, &int16)) {
        *value = int16;
        return true;
    }

    int32_t int32;
    if(cmp_object_as_int(&object, &int32)) {
        *value = int32;
        return true;
    }

    int64_t int64;
    if(cmp_object_as_long(&object, &int64)) {
        *value = int64;
        return true;
    }

    if(cmp_object_as_sinteger(&object, &int64)) {
        *value = int64;
        return true;
    }

    uint8_t uint8;
    if(cmp_object_as_uchar(&object, &uint8)) {
        *value = uint8;
        return true;
    }

    uint16_t uint16;
    if(cmp_object_as_ushort(&object, &uint16)) {
        *value = uint16;
        return true;
    }

    uint32_t uint32;
    if(cmp_object_as_uint(&object, &uint32)) {
        *value = uint32;
        return true;
    }

    uint64_t uint64;
    if(cmp_object_as_ulong(&object, &uint64)) {
        *value = uint64;
        return true;
    }

    if(cmp_object_as_uinteger(&object, &uint64)) {
        *value = uint64;
        return true;
    }

    float float32;
    if(cmp_object_as_float(&object, &float32)) {
        *value = float32;
        return true;
    }

    double float64;
    if(cmp_object_as_double(&object, &float64)) {
        *value = float64;
        return true;
    }

    return false;
}

bool msgpack_read_float32(msgpack_t *msgpack, float *value) {
    *value = 0;

    cmp_object_t object;
    if(!cmp_read_object(&msgpack->cmp, &object)) {
        return false;
    }

    int8_t int8;
    if(cmp_object_as_char(&object, &int8)) {
        *value = int8;
        return true;
    }

    int16_t int16;
    if(cmp_object_as_short(&object, &int16)) {
        *value = int16;
        return true;
    }

    int32_t int32;
    if(cmp_object_as_int(&object, &int32)) {
        *value = int32;
        return true;
    }

    int64_t int64;
    if(cmp_object_as_long(&object, &int64)) {
        *value = int64;
        return true;
    }

    if(cmp_object_as_sinteger(&object, &int64)) {
        *value = int64;
        return true;
    }

    uint8_t uint8;
    if(cmp_object_as_uchar(&object, &uint8)) {
        *value = uint8;
        return true;
    }

    uint16_t uint16;
    if(cmp_object_as_ushort(&object, &uint16)) {
        *value = uint16;
        return true;
    }

    uint32_t uint32;
    if(cmp_object_as_uint(&object, &uint32)) {
        *value = uint32;
        return true;
    }

    uint64_t uint64;
    if(cmp_object_as_ulong(&object, &uint64)) {
        *value = uint64;
        return true;
    }

    if(cmp_object_as_uinteger(&object, &uint64)) {
        *value = uint64;
        return true;
    }

    float float32;
    if(cmp_object_as_float(&object, &float32)) {
        *value = float32;
        return true;
    }

    double float64;
    if(cmp_object_as_double(&object, &float64)) {
        *value = float64;
        return true;
    }

    return false;
}

bool msgpack_read_array_float32(msgpack_t *msgpack,
                                float *values,
                                const uint32_t capacity,
                                uint32_t *size) {
    if(values) {
        memset(values, 0, capacity * sizeof(float));
    }
    if(size) {
        *size = 0;
    }

    uint32_t array_size = 0;
    if(!cmp_read_array(&msgpack->cmp, &array_size)) {
        return false;
    }

    for(uint32_t i = 0; i < array_size; i++) {
        float val = 0;
        if(!msgpack_read_float32(msgpack, &val)) {
            return false;
        }
        if((i < capacity) && values) {
            values[i] = val;
        }
    }

    if(size) {
        *size = array_size;
    }

    return true;
}
