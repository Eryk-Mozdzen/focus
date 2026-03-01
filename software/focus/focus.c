#include <math.h>

#include "focus/api.h"
#include "focus/config.h"
#include "focus/pid.h"
#include "focus/port.h"

typedef struct {
    focus_context_t *context;
    focus_pid_t pid_d;
    focus_pid_t pid_q;
    focus_state_t state;
    focus_state_t requested_state;
    float position_offset;
    float current_offset[3];
    float time_prev;
    float time_panic;
    void *user;
} focus_core_t;

static float min3(const float a, const float b, const float c) {
    return ((a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c));
}

static float max3(const float a, const float b, const float c) {
    return ((a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c));
}

static float clamp(const float x, const float min, const float max) {
    return ((x < min) ? min : ((x > max) ? max : x));
}

static void clark_park_transform(const float i_uvw[3], const float theta, float i_dq[2]) {
    const float i_alpha = i_uvw[0];
    const float i_beta = (0.577350269f * i_uvw[0]) + (1.154700538f * i_uvw[1]);

    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    i_dq[0] = +(i_alpha * cos_theta) + (i_beta * sin_theta);
    i_dq[1] = -(i_alpha * sin_theta) + (i_beta * sin_theta);
}

static void inverse_park_clark_transform(const float u_dq[2], const float theta, float u_uvw[3]) {
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    const float u_alpha = (u_dq[0] * cos_theta) - (u_dq[1] * sin_theta);
    const float u_beta = (u_dq[0] * sin_theta) + (u_dq[1] * cos_theta);

    u_uvw[0] = u_alpha;
    u_uvw[1] = -(0.5f * u_alpha) + (0.866025404f * u_beta);
    u_uvw[2] = -(0.5f * u_alpha) - (0.866025404f * u_beta);
}

static void space_vector_pwm(const float u_uvw[3], const float supply, float duty_cycle_uvw[3]) {
    const float u_min = min3(u_uvw[0], u_uvw[1], u_uvw[2]);
    const float u_max = max3(u_uvw[0], u_uvw[1], u_uvw[2]);

    const float center = (0.5f * supply) - (0.5f * (u_max + u_min));

    duty_cycle_uvw[0] = clamp((u_uvw[0] + center) / supply, 0, 1);
    duty_cycle_uvw[1] = clamp((u_uvw[1] + center) / supply, 0, 1);
    duty_cycle_uvw[2] = clamp((u_uvw[2] + center) / supply, 0, 1);
}

static focus_core_t core;

void focus_init(focus_context_t *context, void *user) {
    core.context = context;
    core.user = user;

    // TODO
    core.current_offset[0] = 0;
    core.current_offset[1] = 0;
    core.current_offset[2] = 0;
    core.position_offset = 0;

    // TODO
    focus_pid_set_kp(&core.pid_d, 1);
    focus_pid_set_ki(&core.pid_d, 1);
    focus_pid_set_kd(&core.pid_d, 0);

    // TODO
    focus_pid_set_kp(&core.pid_q, 1);
    focus_pid_set_ki(&core.pid_q, 1);
    focus_pid_set_kd(&core.pid_q, 0);

    focus_port_init(core.user);

    focus_port_event_panic();
}

void focus_task() {
    const float time = focus_port_timebase(core.user);

    switch(core.state) {
        case FOCUS_STATE_IDLE: {
            switch(core.requested_state) {
                case FOCUS_STATE_RUNNING: {
                    focus_pid_start(&core.pid_d);
                    focus_pid_start(&core.pid_q);
                    focus_port_start(core.user);
                    core.state = FOCUS_STATE_RUNNING;
                } break;
                default: {

                } break;
            }
        } break;
        case FOCUS_STATE_RUNNING: {

        } break;
        case FOCUS_STATE_PANIC: {
            core.context->current_setpoint = 0;
            core.time_prev = time;

            if((time - core.time_panic) >= FOCUS_CONFIG_PANIC_DURATION) {
                core.state = FOCUS_STATE_IDLE;
            }
        } break;
    }
}

void focus_request_state(const focus_state_t requested_state) {
    core.requested_state = requested_state;
}

void focus_port_event_index(const uint32_t encoder) {
    // TODO
    // core.encoder_offset = encoder;
}

void focus_port_event_sample(const focus_port_sample_t *sample) {
    const float position_m =
        6.28318530718f * sample->encoder_count / (FOCUS_CONFIG_ENCODER_CPR - 1);
    const float position_e = FOCUS_CONFIG_MOTOR_POLE_PAIRS * (position_m - core.position_offset);

    const float current_uvw[3] = {
        sample->current_phase_u - core.current_offset[0],
        sample->current_phase_v - core.current_offset[1],
        sample->current_phase_w - core.current_offset[2],
    };

    if(core.state == FOCUS_STATE_RUNNING) {
        /* float current_dq[2];
        clark_park_transform(current_uvw, position_e, current_dq);

        float u_dq[2];
        u_dq[0] = focus_pid_calculate(&core->pid_d, 0, i_dq[0], FOCUS_CONFIG_SAMPLE_PERIOD);
        u_dq[1] = focus_pid_calculate(&core->pid_q, core->context->current_setpoint, i_dq[1],
        FOCUS_CONFIG_SAMPLE_PERIOD);

        float u_uvw[3];
        inverse_park_clark_transform(u_dq, position_e, u_uvw);

        float duty_cycle_uvw[3];
        space_vector_pwm(u_uvw, sample->voltage_vbus, duty_cycle_uvw); */

        const float u_dq[2] = {0, 2.25f};
        float u_uvw[3];
        inverse_park_clark_transform(u_dq, position_e + 1.1f, u_uvw);
        float duty_cycle_uvw[3];
        space_vector_pwm(u_uvw, sample->voltage_vbus, duty_cycle_uvw);

        const focus_port_control_t control = {
            .duty_cycle_u = duty_cycle_uvw[0],
            .duty_cycle_v = duty_cycle_uvw[1],
            .duty_cycle_w = duty_cycle_uvw[2],
        };
        focus_port_control(&control, core.user);
    }

    core.context->supply = sample->voltage_vbus;
    core.context->current_uvw[0] = current_uvw[0];
    core.context->current_uvw[1] = current_uvw[1];
    core.context->current_uvw[2] = current_uvw[2];
    core.context->position = position_m;
}

void focus_port_event_panic() {
    focus_port_shutdown(core.user);

    core.state = FOCUS_STATE_PANIC;
    core.time_panic = focus_port_timebase(core.user);
}
