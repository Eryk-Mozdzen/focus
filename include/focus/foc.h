#ifndef FOCUS_FOC_H
#define FOCUS_FOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "focus/biquad.h"
#include "focus/common.h"
#include "focus/pid.h"

enum focus_foc_state {
    FOCUS_FOC_STATE_NONE,
    FOCUS_FOC_STATE_IDLE,
    FOCUS_FOC_STATE_CALIBRATE_INVERTER,
    FOCUS_FOC_STATE_CALIBRATE_POSITION,
    FOCUS_FOC_STATE_CALIBRATE_MOTOR,
    FOCUS_FOC_STATE_CALIBRATE_FULL,
    FOCUS_FOC_STATE_RUNNING,
};

struct focus_foc {
    struct focus_srv_control srv;

    struct {
        float rs;
        float ld;
        float lq;
        float kv;
        uint32_t npp;
        bool enable_identification;
    } params;

    volatile float position;
    volatile float velocity;
    volatile float voltage;

    float i_dq_setpoint[2];
    struct focus_biquad i_dq_filter[2];
    struct focus_pid pid_dq[2];

    float current_state_enter_time;
    enum focus_foc_state state_requested;
    enum focus_foc_state state_current;
};

void focus_foc_init(struct focus_foc *foc);
void focus_foc_req_state(struct focus_foc *foc, enum focus_foc_state state);
void focus_foc_set_torque(struct focus_foc *foc, float torque);
float focus_foc_get_position(struct focus_foc *foc);
float focus_foc_get_velocity(struct focus_foc *foc);
float focus_foc_get_voltage(struct focus_foc *foc);
void focus_foc_task(struct focus_foc *foc);

void focus_foc_driver(struct focus_srv_control *srv, struct focus_event *event);

#ifdef __cplusplus
}
#endif

#endif
