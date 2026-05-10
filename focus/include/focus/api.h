#ifndef FOCUS_API_H
#define FOCUS_API_H

#include <stdint.h>

#include "focus/config.h"

typedef struct {
    struct {
        float rs;
        float ld;
        float lq;
    } motor;
    float current_offset[3];
    float current_scale[3];
#ifdef FOCUS_CONFIG_ENCODER_ABI
    int32_t encoder_lut[FOCUS_CONFIG_ENCODER_CPR];
#endif
} focus_calibration_t;

typedef enum {
    FOCUS_REQUESTED_STATE_IDLE = 10,
    FOCUS_REQUESTED_STATE_CALIBRATE_CURRENT,
#ifdef FOCUS_CONFIG_ENCODER_ABI
    FOCUS_REQUESTED_STATE_CALIBRATE_ENCODER,
#endif
    FOCUS_REQUESTED_STATE_CALIBRATE_MOTOR,
    FOCUS_REQUESTED_STATE_CLOSE_LOOP,
} focus_requested_state_t;

void focus_init(void *user);
void focus_task();

void focus_request_state(const uint32_t motor, const focus_requested_state_t requested_state);
focus_calibration_t *focus_calibration_data(const uint32_t motor);
void focus_calibration_update(const uint32_t motor);

void focus_set_torque(const uint32_t motor, const float torque);
float focus_get_position(const uint32_t motor);
float focus_get_velocity(const uint32_t motor);

#endif
