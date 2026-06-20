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
#ifndef FOCUS_CONFIG_ENCODER_TYPE_AB
        int32_t eccentricity_lookup[FOCUS_CONFIG_ENCODER_CPR];
#endif
#endif
    } encoder;
#endif
} focus_api_calibration_t;

typedef enum {
    FOCUS_API_STATE_IDLE = 10,
    FOCUS_API_STATE_CALIBRATE_CURRENT,
#ifdef FOCUS_CONFIG_ENCODER_ENABLE
    FOCUS_API_STATE_CALIBRATE_ENCODER,
#endif
    FOCUS_API_STATE_CALIBRATE_MOTOR,
    FOCUS_API_STATE_RUNNING,
} focus_api_state_t;

typedef void (*focus_api_state_ended_t)(const uint32_t, const focus_api_state_t, void *);

void focus_api_init(void *user);
void focus_api_task();

focus_api_calibration_t *focus_api_calibration(const uint32_t motor);
void focus_api_calibration_update(const uint32_t motor);

void focus_api_state_request(const uint32_t motor,
                             const focus_api_state_t state_requested,
                             const focus_api_state_ended_t state_ended_callback);
void focus_api_torque_set(const uint32_t motor, const float torque);
#ifdef FOCUS_CONFIG_ENCODER_ENABLE
float focus_api_position(const uint32_t motor);
#endif
float focus_api_velocity(const uint32_t motor);

#endif
