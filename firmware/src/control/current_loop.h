#ifndef CURRENT_LOOP_H
#define CURRENT_LOOP_H

#include <stdbool.h>

#include "control/pid.h"
#include "system/if_encoder.h"
#include "system/if_inverter.h"

typedef struct {
    if_inverter_t *inverter;
    if_encoder_t *encoder;

    pid_t pi_controller_q;
    pid_t pi_controller_d;

    float overcurrent;
    float overvoltage;
    float undervoltage;

    float current_setpoint;

    bool supply_ready;
    float supply;

    bool current_ready;
    float current[3];

    bool mech_position_ready;
    float mech_position;
    float mech_position_offset;
    uint32_t motor_pole_pairs;
} current_loop_t;

#endif
