#include "focus/encoder.h"

status_t encoder_init(encoder_if_t *encoder) {
    if(!encoder->init) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return encoder->init(encoder->driver);
}

status_t encoder_deinit(encoder_if_t *encoder) {
    if(!encoder->deinit) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return encoder->deinit(encoder->driver);
}

status_t encoder_sample_start(encoder_if_t *encoder) {
    if(!encoder->sample_start) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return encoder->sample_start(encoder->driver);
}

status_t encoder_sample_get(encoder_if_t *encoder, float *sample) {
    if(!encoder->sample_get) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    if(!sample) {
        return STATUS_SYSTEM_INVALID_ARG;
    }

    return encoder->sample_get(encoder->driver, sample);
}

void encoder_set_handler(encoder_if_t *encoder, const encoder_handler_t handler, void *context) {
    encoder->handler_context = context;
    encoder->handler = handler;
}
