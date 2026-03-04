#include <math.h>

#include "focus/api.h"
#include "focus/config.h"
#include "focus/pid.h"
#include "focus/port.h"

#define PI 3.14159265359f

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
    float time_calibrate;
    void *user;
} focus_core_t;

static float clamp(const float x, const float min, const float max) {
    return ((x < min) ? min : ((x > max) ? max : x));
}

static float wrap(const float in) {
    return atan2f(sinf(in), cosf(in));
}

static void clark_transform(const float i_uvw[3], float i_ab[2]) {
    i_ab[0] = i_uvw[0];
    i_ab[1] = (0.577350269f * i_uvw[0]) + (1.154700538f * i_uvw[1]);
}

static void park_transform(const float i_ab[2], const float theta, float i_dq[2]) {
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    i_dq[0] = +(i_ab[0] * cos_theta) + (i_ab[1] * sin_theta);
    i_dq[1] = -(i_ab[0] * sin_theta) + (i_ab[1] * cos_theta);
}

static void inverse_park_transform(const float u_dq[2], const float theta, float u_ab[2]) {
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    u_ab[0] = (u_dq[0] * cos_theta) - (u_dq[1] * sin_theta);
    u_ab[1] = (u_dq[0] * sin_theta) + (u_dq[1] * cos_theta);
}

static void inverse_clark_transform(const float u_ab[2], float u_uvw[3]) {
    u_uvw[0] = u_ab[0];
    u_uvw[1] = -(0.5f * u_ab[0]) + (0.866025404f * u_ab[1]);
    u_uvw[2] = -(0.5f * u_ab[0]) - (0.866025404f * u_ab[1]);
}

static void svpwm(const float u_ab[2], float u_supply, float duty_cycle_uvw[3]) {
    const float u_alpha = u_ab[0] / u_supply;
    const float u_beta = u_ab[1] / u_supply;

    uint8_t sector;
    if(u_beta >= 0.f) {
        if(u_alpha >= 0.f) {
            sector = ((0.577350269f * u_beta) > u_alpha) ? 2 : 1;
        } else {
            sector = ((-0.577350269f * u_beta) > u_alpha) ? 3 : 2;
        }
    } else {
        if(u_alpha >= 0.f) {
            sector = ((-0.577350269f * u_beta) > u_alpha) ? 5 : 6;
        } else {
            sector = ((0.577350269f * u_beta) > u_alpha) ? 4 : 5;
        }
    }

    switch(sector) {
        case 1: {
            const float t1 = (u_alpha - (0.577350269f * u_beta));
            const float t2 = (1.154700538f * u_beta);
            duty_cycle_uvw[0] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[1] = duty_cycle_uvw[0] - t1;
            duty_cycle_uvw[2] = duty_cycle_uvw[1] - t2;
        } break;
        case 2: {
            const float t1 = (u_alpha + (0.577350269f * u_beta));
            const float t2 = (-u_alpha + (0.577350269f * u_beta));
            duty_cycle_uvw[1] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[0] = duty_cycle_uvw[1] - t2;
            duty_cycle_uvw[2] = duty_cycle_uvw[0] - t1;
        } break;
        case 3: {
            const float t1 = (1.154700538f * u_beta);
            const float t2 = (-u_alpha - (0.577350269f * u_beta));
            duty_cycle_uvw[1] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[2] = duty_cycle_uvw[1] - t1;
            duty_cycle_uvw[0] = duty_cycle_uvw[2] - t2;
        } break;
        case 4: {
            const float t1 = (-u_alpha + (0.577350269f * u_beta));
            const float t2 = (-1.154700538f * u_beta);
            duty_cycle_uvw[2] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[1] = duty_cycle_uvw[2] - t2;
            duty_cycle_uvw[0] = duty_cycle_uvw[1] - t1;
        } break;
        case 5: {
            const float t1 = (-u_alpha - (0.577350269f * u_beta));
            const float t2 = (u_alpha - (0.577350269f * u_beta));
            duty_cycle_uvw[2] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[0] = duty_cycle_uvw[2] - t1;
            duty_cycle_uvw[1] = duty_cycle_uvw[0] - t2;
        } break;
        case 6: {
            const float t1 = (-1.154700538f * u_beta);
            const float t2 = (u_alpha + (0.577350269f * u_beta));
            duty_cycle_uvw[0] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[2] = duty_cycle_uvw[0] - t2;
            duty_cycle_uvw[1] = duty_cycle_uvw[2] - t1;
        } break;
    }

    duty_cycle_uvw[0] = clamp(duty_cycle_uvw[0], 0.f, 1.f);
    duty_cycle_uvw[1] = clamp(duty_cycle_uvw[1], 0.f, 1.f);
    duty_cycle_uvw[2] = clamp(duty_cycle_uvw[2], 0.f, 1.f);
}

static focus_core_t core;

void focus_init(focus_context_t *context, void *user) {
    core.context = context;
    core.user = user;

    // TODO
    core.current_offset[0] = 0.16f;
    core.current_offset[1] = 0.22f;
    core.current_offset[2] = 0.23f;
    core.position_offset = 0.f;

    // TODO
    const float Rs = 0.13144f;
    const float Ld = 0.000135f;
    const float Lq = 0.000135f;

    const float f = 1000.f;
    const float w = 2.f * PI * f;

    focus_pid_set_kp(&core.pid_d, w * Ld);
    focus_pid_set_ki(&core.pid_d, w * Rs);
    focus_pid_set_kd(&core.pid_d, 0);
    focus_pid_antiwindup_enable(&core.pid_d, false);

    focus_pid_set_kp(&core.pid_q, w * Lq);
    focus_pid_set_ki(&core.pid_q, w * Rs);
    focus_pid_set_kd(&core.pid_q, 0);
    focus_pid_antiwindup_enable(&core.pid_q, false);

    focus_port_init(core.user);

    focus_port_event_panic();
}

void focus_task() {
    const float time = focus_port_timebase(core.user);

    switch(core.state) {
        case FOCUS_STATE_IDLE: {
            switch(core.requested_state) {
                case FOCUS_STATE_CALIBRATE_ENCODER_BEGIN: {
                    core.context->position_open_loop = 0;
                    core.time_calibrate = time;
                    core.state = FOCUS_STATE_CALIBRATE_ENCODER_INDEX;
                } break;
                case FOCUS_STATE_RUNNING: {
                    focus_pid_start(&core.pid_d);
                    focus_pid_start(&core.pid_q);
                    focus_port_start(core.user);
                    core.state = FOCUS_STATE_RUNNING;
                } break;
                default: {

                } break;
            }
            core.requested_state = FOCUS_STATE_IDLE;
        } break;
        case FOCUS_STATE_PANIC: {
            core.context->current_setpoint = 0;
            core.time_prev = time;

            if((time - core.time_panic) >= FOCUS_CONFIG_PANIC_DURATION) {
                core.state = FOCUS_STATE_IDLE;
            }
        } break;
        default: {

        } break;
    }
}

void focus_request_state(const focus_state_t requested_state) {
    core.requested_state = requested_state;
}

void focus_port_event_index(const uint32_t encoder) {
    (void)encoder;

    const float time = focus_port_timebase(core.user);

    switch(core.state) {
        case FOCUS_STATE_CALIBRATE_ENCODER_INDEX: {
            if((time - core.time_calibrate) > 0.1f) {
                core.time_calibrate = time;
                core.position_offset = 0;
                core.state = FOCUS_STATE_CALIBRATE_ENCODER_ZERO;
            }
        } break;
        default: {

        } break;
    }
}

extern volatile float scope_buffer[1000][3];
extern volatile uint32_t scope_index;

void focus_port_event_sample(const focus_port_sample_t *sample) {
    const float time = focus_port_timebase(core.user);

    const float position_m = wrap(
        (2.f * PI * sample->encoder_count / (FOCUS_CONFIG_ENCODER_CPR - 1)) - core.position_offset);

    /*float current_uvw[3] = {
        sample->current_phase_u - core.current_offset[0],
        sample->current_phase_v - core.current_offset[1],
        sample->current_phase_w - core.current_offset[2],
    };
    const float current_uvw_mean = 0.3334f * (current_uvw[0] + current_uvw[1] + current_uvw[2]);
    current_uvw[0] -= current_uvw_mean;
    current_uvw[1] -= current_uvw_mean;
    current_uvw[2] -= current_uvw_mean;*/

    const float i_uvw[3] = {
        sample->current_phase_u - core.current_offset[0],
        sample->current_phase_v - core.current_offset[1],
        -(sample->current_phase_u + sample->current_phase_v - core.current_offset[0] -
          core.current_offset[1]),
    };

    switch(core.state) {
        case FOCUS_STATE_CALIBRATE_ENCODER_INDEX: {
            core.context->position_open_loop =
                wrap(core.context->position_open_loop +
                     (FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY * FOCUS_CONFIG_SAMPLE_PERIOD));
            const float position_open_loop_e =
                wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * core.context->position_open_loop);

            const float u_dq[2] = {0, FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE};
            float u_ab[2];
            inverse_park_transform(u_dq, position_open_loop_e, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->voltage_vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_ZERO: {
            const float u_dq[2] = {FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0};
            float u_ab[2];
            inverse_park_transform(u_dq, 0, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->voltage_vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);

            if((time - core.time_calibrate) >= FOCUS_CONFIG_CALIBRATE_ENCODER_ZERO_TIME) {
                core.position_offset = position_m;
                focus_port_event_panic();
            }
        } break;
        case FOCUS_STATE_RUNNING: {
            const float position_e = wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * position_m);

            float i_ab[2];
            clark_transform(i_uvw, i_ab);
            float i_dq[2];
            park_transform(i_ab, position_e, i_dq);
            const float u_dq[2] = {
                focus_pid_calculate(&core.pid_d, 0.f, i_dq[0], FOCUS_CONFIG_SAMPLE_PERIOD),
                focus_pid_calculate(&core.pid_q, 0.2f, i_dq[1], FOCUS_CONFIG_SAMPLE_PERIOD),
            };
            float u_ab[2];
            inverse_park_transform(u_dq, position_e, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->voltage_vbus, duty_cycle_uvw);

            if(scope_index < 1000) {
                scope_buffer[scope_index][0] = i_dq[0];
                scope_buffer[scope_index][1] = i_dq[1];
                scope_buffer[scope_index][2] = 0;
                scope_index++;
            }

            /*core.context->position_open_loop =
                wrap(core.context->position_open_loop + (50.f * FOCUS_CONFIG_SAMPLE_PERIOD));
            const float position_open_loop_e =
                wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * core.context->position_open_loop);

            const float u_dq[2] = {0, 4.f};
            float u_ab[2];
            inverse_park_transform(u_dq, position_open_loop_e, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->voltage_vbus, duty_cycle_uvw);

            core.context->ab[0] = u_ab[0];
            core.context->ab[1] = u_ab[1];

            core.context->svpwm[0] = duty_cycle_uvw[0];
            core.context->svpwm[1] = duty_cycle_uvw[1];
            core.context->svpwm[2] = duty_cycle_uvw[2];*/

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);
        } break;
        default: {

        } break;
    }

    core.context->supply = sample->voltage_vbus;
    core.context->position = position_m;
}

void focus_port_event_panic() {
    focus_port_shutdown(core.user);

    core.state = FOCUS_STATE_PANIC;
    core.time_panic = focus_port_timebase(core.user);
}
