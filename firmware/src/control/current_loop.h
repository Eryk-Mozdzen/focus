#ifndef CURRENT_LOOP_H
#define CURRENT_LOOP_H

#include <stdbool.h>

#include "system/if_encoder.h"
#include "system/if_inverter.h"
#include "system/status.h"

typedef struct {
    if_inverter_t *inverter;
    if_encoder_t *encoder;

    float current_max;
    float iq_ref;

    bool current_ready;
    float current[3];

    bool position_ready;
    float position;
} current_loop_t;

status_t current_loop_start(current_loop_t *cl);
status_t current_loop_set_current_max(current_loop_t *cl, const float current_max);
status_t current_loop_set_current_setpoint(current_loop_t *cl, const float current_setpoint);

#endif
