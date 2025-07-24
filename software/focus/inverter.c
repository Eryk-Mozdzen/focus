#include "focus/inverter.h"

status_t inverter_init(inverter_if_t *inverter) {
    if(!inverter->init) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return inverter->init(inverter->driver);
}

status_t inverter_deinit(inverter_if_t *inverter) {
    if(!inverter->deinit) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return inverter->deinit(inverter->driver);
}

status_t
inverter_set_duty_cycle(inverter_if_t *inverter, const float u, const float v, const float w) {
    if(!inverter->set_duty_cycle) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    return inverter->set_duty_cycle(inverter->driver, u, v, w);
}

status_t inverter_get_current(inverter_if_t *inverter, float *u, float *v, float *w) {
    if(!inverter->get_current) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    if(!u || !v || !w) {
        return STATUS_SYSTEM_INVALID_ARG;
    }

    return inverter->get_current(inverter->driver, u, v, w);
}

status_t inverter_get_supply(inverter_if_t *inverter, float *supply) {
    if(!inverter->get_supply) {
        return STATUS_SYSTEM_INVALID_IMPL;
    }

    if(!supply) {
        return STATUS_SYSTEM_INVALID_ARG;
    }

    return inverter->get_supply(inverter->driver, supply);
}

void inverter_set_handler(inverter_if_t *inverter,
                          const inverter_handler_t handler,
                          void *context) {
    inverter->handler_context = context;
    inverter->handler = handler;
}
