#ifndef FOCUS_INVERTER_H
#define FOCUS_INVERTER_H

#include <stdint.h>

#include "focus/status.h"

typedef enum {
    INVERTER_EVENT_SAMPLE_START,
    INVERTER_EVENT_SAMPLE_READY,
} inverter_event_t;

typedef void (*inverter_handler_t)(const inverter_event_t, void *);

typedef struct {
    void *driver;
    status_t (*init)(void *);
    status_t (*deinit)(void *);
    status_t (*set_duty_cycle)(void *, const float, const float, const float);
    status_t (*get_current)(void *, float *, float *, float *);
    status_t (*get_supply)(void *, float *);

    void *handler_context;
    inverter_handler_t handler;
} inverter_if_t;

status_t inverter_init(inverter_if_t *inverter);
status_t inverter_deinit(inverter_if_t *inverter);
status_t
inverter_set_duty_cycle(inverter_if_t *inverter, const float u, const float v, const float w);
status_t inverter_get_current(inverter_if_t *inverter, float *u, float *v, float *w);
status_t inverter_get_supply(inverter_if_t *inverter, float *supply);

void inverter_set_handler(inverter_if_t *inverter, const inverter_handler_t handler, void *context);

#endif
