#include <math.h>
#include <string.h>

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
    focus_callback_t requested_state_complete;

    uint16_t encoder_lut[FOCUS_CONFIG_ENCODER_CPR];
    uint16_t encoder_lut_last;

    float current_offset[3];
    float current_scale[3];
    uint32_t current_offset_num;

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

static uint16_t encoder_linear_interpolation(const uint16_t x1,
                                             const uint16_t y1,
                                             const uint16_t x2,
                                             const uint16_t y2,
                                             const uint16_t xi) {
    uint16_t result = 0;

    result = y1 + (((y2 - y1) * (xi - x1)) / (x2 - x1));

    return result;
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

static void
open_loop(focus_core_t *core, const float ud, const float uq, const float vbus, const float theta) {
    const float u_dq[2] = {ud, uq};
    float u_ab[2];
    inverse_park_transform(u_dq, theta, u_ab);
    float duty_cycle_uvw[3];
    svpwm(u_ab, vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(&control, core->user);
}

static void call_complete(focus_core_t *core) {
    if(core->requested_state_complete) {
        core->requested_state_complete(core->user);
    }
}

static focus_core_t core;

void focus_init(focus_context_t *context, void *user) {
    core.context = context;
    core.user = user;

    core.current_offset[0] = 0.f;
    core.current_offset[1] = 0.f;
    core.current_offset[2] = 0.f;
    core.current_scale[0] = 1.15f; // TODO
    core.current_scale[1] = 1.07f; // TODO
    core.current_scale[2] = 1.f;   // TODO
    memset(core.encoder_lut, 0, sizeof(core.encoder_lut));

    const float Rs = 0.13144f;  // TODO
    const float Ld = 0.000135f; // TODO
    const float Lq = 0.000135f; // TODO

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
                case FOCUS_STATE_CALIBRATE_CURRENT: {
                    core.current_offset[0] = 0.f;
                    core.current_offset[1] = 0.f;
                    core.current_offset[2] = 0.f;
                    core.current_offset_num = 0;
                    core.time_calibrate = time;
                    core.state = FOCUS_STATE_CALIBRATE_CURRENT;
                } break;
                case FOCUS_STATE_CALIBRATE_ENCODER_BEGIN: {
                    core.context->position_open_loop = 0;
                    core.time_calibrate = time;
                    core.encoder_lut_last = 0;
                    memset(core.encoder_lut, 0, sizeof(core.encoder_lut));
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

void focus_request_state(const focus_state_t requested_state, const focus_callback_t callback) {
    core.requested_state = requested_state;
    core.requested_state_complete = callback;
}

void focus_port_event_index(const uint32_t encoder) {
    (void)encoder;

    const float time = focus_port_timebase(core.user);

    switch(core.state) {
        case FOCUS_STATE_CALIBRATE_ENCODER_INDEX: {
            if((time - core.time_calibrate) > 0.1f) {
                core.time_calibrate = time;
                core.context->position_open_loop = 0;
                core.state = FOCUS_STATE_CALIBRATE_ENCODER_ZERO;
            }
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY: {
            if((time - core.time_calibrate) >
               (0.5f * 2.f * PI * FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY)) {
                call_complete(&core);
                focus_port_event_panic();
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

    const float position_m =
        wrap((2.f * PI * (sample->encoder_count - core.encoder_lut[sample->encoder_count]) /
              (FOCUS_CONFIG_ENCODER_CPR - 1)));

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
        core.current_scale[0] * (sample->current_phase_u - core.current_offset[0]),
        core.current_scale[1] * (sample->current_phase_v - core.current_offset[1]),
        core.current_scale[2] * (sample->current_phase_w - core.current_offset[2]),
    };

    switch(core.state) {
        case FOCUS_STATE_CALIBRATE_CURRENT: {
            core.current_offset[0] =
                ((core.current_offset[0] * core.current_offset_num) + sample->current_phase_u) /
                (core.current_offset_num + 1);
            core.current_offset[1] =
                ((core.current_offset[1] * core.current_offset_num) + sample->current_phase_v) /
                (core.current_offset_num + 1);
            core.current_offset[2] =
                ((core.current_offset[2] * core.current_offset_num) + sample->current_phase_w) /
                (core.current_offset_num + 1);

            core.current_offset_num++;

            if((time - core.time_calibrate) >= FOCUS_CONFIG_CALIBRATE_CURRENT_TIME) {
                call_complete(&core);
                focus_port_event_panic();
            }
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_INDEX: {
            core.context->position_open_loop =
                wrap(core.context->position_open_loop +
                     (FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY * FOCUS_CONFIG_SAMPLE_PERIOD));
            const float position_open_loop_e =
                wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * core.context->position_open_loop);

            open_loop(&core, FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0, sample->voltage_vbus,
                      position_open_loop_e);
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_ZERO: {
            open_loop(&core, FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0, sample->voltage_vbus, 0);

            if((time - core.time_calibrate) >= FOCUS_CONFIG_CALIBRATE_ENCODER_ZERO_TIME) {
                core.context->position_open_loop = 0;
                core.time_calibrate = time;
                core.state = FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY;
            }
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY: {
            core.context->position_open_loop =
                wrap(core.context->position_open_loop +
                     (FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY * FOCUS_CONFIG_SAMPLE_PERIOD));
            const float open_loop_e =
                wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * core.context->position_open_loop);

            open_loop(&core, FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0, sample->voltage_vbus,
                      open_loop_e);

            const uint16_t open_loop_count =
                ((uint16_t)((FOCUS_CONFIG_ENCODER_CPR *
                             clamp((wrap(core.context->position_open_loop) + PI) / (2.f * PI), 0.f,
                                   1.f)) +
                            (FOCUS_CONFIG_ENCODER_CPR / 2))) %
                FOCUS_CONFIG_ENCODER_CPR;

            const uint16_t encoder_lut_curr = sample->encoder_count % FOCUS_CONFIG_ENCODER_CPR;

            core.encoder_lut[encoder_lut_curr] = sample->encoder_count - open_loop_count;

            for(uint16_t i = core.encoder_lut_last; i < encoder_lut_curr; i++) {
                core.encoder_lut[i] = encoder_linear_interpolation(
                    core.encoder_lut_last, core.encoder_lut[core.encoder_lut_last],
                    encoder_lut_curr, core.encoder_lut[encoder_lut_curr], i);
            }
            core.encoder_lut_last = encoder_lut_curr;
        } break;
        case FOCUS_STATE_RUNNING: {
            const float position_e = wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * position_m);

            float i_ab[2];
            clark_transform(i_uvw, i_ab);
            float i_dq[2];
            park_transform(i_ab, position_e, i_dq);
            const float u_dq[2] = {
                focus_pid_calculate(&core.pid_d, 0.f, i_dq[0], FOCUS_CONFIG_SAMPLE_PERIOD),
                focus_pid_calculate(&core.pid_q, 1.f, i_dq[1], FOCUS_CONFIG_SAMPLE_PERIOD),
            };
            float u_ab[2];
            inverse_park_transform(u_dq, position_e, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->voltage_vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);

            if(scope_index < 1000) {
                scope_buffer[scope_index][0] = i_dq[0];
                scope_buffer[scope_index][1] = i_dq[1];
                scope_buffer[scope_index][2] = position_m;
                scope_index++;
            }

            /*core.context->position_open_loop =
                wrap(core.context->position_open_loop + (1.f * FOCUS_CONFIG_SAMPLE_PERIOD));
            const float open_loop_e =
                wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * core.context->position_open_loop);

            open_loop(&core, FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0, sample->voltage_vbus,
                      open_loop_e);*/
            /*float u_uvw[3];
            inverse_clark_transform(u_ab, u_uvw);
            const float duty_cycle_uvw[3] = {
                clamp((u_uvw[0] / sample->voltage_vbus) + 0.5f, 0.f, 1.f),
                clamp((u_uvw[1] / sample->voltage_vbus) + 0.5f, 0.f, 1.f),
                clamp((u_uvw[2] / sample->voltage_vbus) + 0.5f, 0.f, 1.f),
            };*/

            /*if(scope_index < 1000) {
                scope_buffer[scope_index][0] = i_uvw[0];
                scope_buffer[scope_index][1] = i_uvw[1];
                scope_buffer[scope_index][2] = i_uvw[2];
                scope_index++;
            }*/

            /*core.context->ab[0] = u_ab[0];
            core.context->ab[1] = u_ab[1];

            core.context->svpwm[0] = duty_cycle_uvw[0];
            core.context->svpwm[1] = duty_cycle_uvw[1];
            core.context->svpwm[2] = duty_cycle_uvw[2];*/
        } break;
        default: {

        } break;
    }

    core.context->uvw[0] = i_uvw[0];
    core.context->uvw[1] = i_uvw[1];
    core.context->uvw[2] = i_uvw[2];

    core.context->supply = sample->voltage_vbus;
    core.context->position = position_m;
}

void focus_port_event_panic() {
    focus_port_shutdown(core.user);

    core.state = FOCUS_STATE_PANIC;
    core.time_panic = focus_port_timebase(core.user);
}
