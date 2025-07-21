#ifndef IF_ENCODER_H
#define IF_ENCODER_H

#include <stdint.h>

#include "system/status.h"

typedef enum {
    ENCODER_EVENT_ERROR,
    ENCODER_EVENT_SAMPLE_READY,
} encoder_event_t;

typedef void (*encoder_handler_t)(const encoder_event_t, void *);

typedef struct {
    void *driver;
    status_t (*init)(void *);
    status_t (*deinit)(void *);
    status_t (*sample_start)(void *);
    status_t (*sample_get)(void *, float *);

    void *handler_context;
    encoder_handler_t handler;
} if_encoder_t;

status_t encoder_init(if_encoder_t *encoder);
status_t encoder_deinit(if_encoder_t *encoder);
status_t encoder_sample_start(if_encoder_t *encoder);
status_t encoder_sample_get(if_encoder_t *encoder, float *sample);

void encoder_set_handler(if_encoder_t *encoder, const encoder_handler_t handler, void *context);

#endif
