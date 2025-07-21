#ifndef IF_INVERTER_H
#define IF_INVERTER_H

#include <stdint.h>

#include "system/status.h"

typedef enum {
    INVERTER_EVENT_ERROR,
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

    void *handler_context;
    inverter_handler_t handler;
} if_inverter_t;

status_t inverter_init(if_inverter_t *inverter);
status_t inverter_deinit(if_inverter_t *inverter);
status_t
inverter_set_duty_cycle(if_inverter_t *inverter, const float u, const float v, const float w);
status_t inverter_get_current(if_inverter_t *inverter, float *u, float *v, float *w);

void inverter_set_handler(if_inverter_t *inverter, const inverter_handler_t handler, void *context);

#endif
