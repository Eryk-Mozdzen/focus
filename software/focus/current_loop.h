#ifndef FOCUS_CURRENT_LOOP_H
#define FOCUS_CURRENT_LOOP_H

#include <stdbool.h>

#include "focus/encoder.h"
#include "focus/inverter.h"
#include "focus/memory_reg.h"
#include "focus/pid.h"

typedef struct {
    inverter_if_t *inverter;
    encoder_if_t *encoder;

    pid_t pi_controller_q;
    pid_t pi_controller_d;

    memory_reg_t overcurrent;
    memory_reg_t overvoltage;
    memory_reg_t undervoltage;

    bool supply_ready;
    memory_reg_t supply;

    bool current_ready;
    memory_reg_t current[3];
    memory_reg_t current_setpoint;

    bool position_ready;
    memory_reg_t position;
    memory_reg_t position_offset;
    memory_reg_t motor_pole_pairs;
} current_loop_t;

#endif
