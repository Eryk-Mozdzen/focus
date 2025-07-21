#include "system/if_inverter.h"

status_t inverter_init(if_inverter_t *inverter) {
    if(!inverter->init) {
        return 1;
    }

    return inverter->init(inverter->driver);
}

status_t inverter_deinit(if_inverter_t *inverter) {
    if(!inverter->deinit) {
        return 1;
    }

    return inverter->deinit(inverter->driver);
}

status_t
inverter_set_duty_cycle(if_inverter_t *inverter, const float u, const float v, const float w) {
    if(!inverter->set_duty_cycle) {
        return 1;
    }

    const float u_clamped = (u > 1.f) ? 1.f : ((u < 0.f) ? 0.f : u);
    const float v_clamped = (v > 1.f) ? 1.f : ((v < 0.f) ? 0.f : v);
    const float w_clamped = (w > 1.f) ? 1.f : ((w < 0.f) ? 0.f : w);

    return inverter->set_duty_cycle(inverter->driver, u_clamped, v_clamped, w_clamped);
}

status_t inverter_get_current(if_inverter_t *inverter, float *u, float *v, float *w) {
    if(!inverter->get_current) {
        return 1;
    }

    if(!u || !v || !w) {
        return 2;
    }

    return inverter->get_current(inverter->driver, u, v, w);
}

void inverter_set_handler(if_inverter_t *inverter,
                          const inverter_handler_t handler,
                          void *context) {
    inverter->handler_context = context;
    inverter->handler = handler;
}
