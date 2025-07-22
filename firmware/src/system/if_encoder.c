#include "system/if_encoder.h"

status_t encoder_init(if_encoder_t *encoder) {
    if(!encoder->init) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return encoder->init(encoder->driver);
}

status_t encoder_deinit(if_encoder_t *encoder) {
    if(!encoder->deinit) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return encoder->deinit(encoder->driver);
}

status_t encoder_sample_start(if_encoder_t *encoder) {
    if(!encoder->sample_start) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return encoder->sample_start(encoder->driver);
}

status_t encoder_sample_get(if_encoder_t *encoder, float *sample) {
    if(!encoder->sample_get) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    if(!sample) {
        return STATUS_SYSTEM_INVALID_ARG;
    }

    return encoder->sample_get(encoder->driver, sample);
}

void encoder_set_handler(if_encoder_t *encoder, const encoder_handler_t handler, void *context) {
    encoder->handler_context = context;
    encoder->handler = handler;
}
