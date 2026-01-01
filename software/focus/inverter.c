#include "focus.h"

focus_status_t focus_inverter_reset(focus_inverter_t *inverter) {
    if(!inverter->reset) {
        FOCUS_LOG("inverter reset: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    const uint32_t ret = inverter->reset(inverter->user);

    if(ret) {
        FOCUS_LOG("inverter reset: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t
focus_inverter_set_gates(focus_inverter_t *inverter, const float u, const float v, const float w) {
    if(!inverter->set_gates) {
        FOCUS_LOG("inverter set_gates: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    const uint32_t ret = inverter->set_gates(u, v, w, inverter->user);

    if(ret) {
        FOCUS_LOG("inverter set_gates: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}

focus_status_t
focus_inverter_get_current(focus_inverter_t *inverter, float *u, float *v, float *w) {
    if(!inverter->get_current) {
        FOCUS_LOG("inverter get_current: not implemented");
        return FOCUS_STATUS_INVALID_DRIVER;
    }

    if((!u) || (!v) || (!w)) {
        FOCUS_LOG("inverter get_current: null destination");
        return FOCUS_STATUS_INVALID_ARG;
    }

    const uint32_t ret = inverter->get_current(u, v, w, inverter->user);

    if(ret) {
        FOCUS_LOG("inverter get_current: %lu", ret);
        return FOCUS_STATUS_DRIVER_ERROR;
    }

    return FOCUS_STATUS_SUCCESS;
}
