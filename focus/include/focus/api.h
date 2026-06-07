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
    struct {
        float offset[3];
        float scale[3];
    } current;
#ifdef FOCUS_CONFIG_ENCODER_ENABLE
    struct {
        uint32_t align_offset;
#ifdef FOCUS_CONFIG_ENCODER_ECCENTRICITY_ENABLE
        int32_t eccentricity_lookup_table[FOCUS_CONFIG_ENCODER_CPR];
#endif
    } encoder;
#endif
} focus_calibration_t;

typedef enum {
    FOCUS_REQUESTED_STATE_IDLE = 10,
    FOCUS_REQUESTED_STATE_CALIBRATE_CURRENT,
#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#ifndef FOCUS_CONFIG_ENCODER_AB
    FOCUS_REQUESTED_STATE_CALIBRATE_ENCODER,
#endif
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
#ifndef FOCUS_CONFIG_SENSORLESS_ENABLE
float focus_get_position(const uint32_t motor);
#endif
float focus_get_velocity(const uint32_t motor);

#endif
