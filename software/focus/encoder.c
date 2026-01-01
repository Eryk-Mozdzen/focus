#include "focus.h"

focus_status_t focus_encoder_reset(focus_encoder_t *encoder) {
    if(!encoder->reset) {
        FOCUS_LOG("encoder reset: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    const uint32_t ret = encoder->reset(encoder->user);

    if(ret) {
        FOCUS_LOG("encoder reset: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t encoder_sample_start(focus_encoder_t *encoder) {
    if(!encoder->sample_start) {
        FOCUS_LOG("encoder sample_start: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    const uint32_t ret = encoder->sample_start(encoder->user);

    if(ret) {
        FOCUS_LOG("encoder sample_start: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t encoder_sample_get(focus_encoder_t *encoder, float *sample) {
    if(!encoder->sample_get) {
        FOCUS_LOG("encoder sample_get: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    if(!sample) {
        FOCUS_LOG("encoder sample_get: null destination");
        return FOCUS_STATUS_INVALID_ARG;
    }

    const uint32_t ret = encoder->sample_get(sample, encoder->user);

    if(ret) {
        FOCUS_LOG("encoder sample_get: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}
