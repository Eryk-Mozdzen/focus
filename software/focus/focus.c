#include <math.h>
#include <string.h>

#include "focus/api.h"
#include "focus/config.h"
#include "focus/pid.h"
#include "focus/port.h"

#define PI 3.14159265359f

#define ENCODER_TO_MECH(count)                                                                     \
    (wrap((((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR) <= (FOCUS_CONFIG_ENCODER_CPR / 2))      \
         ? (((2.f * PI) / FOCUS_CONFIG_ENCODER_CPR) *                                              \
            (((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR))                                      \
         : ((((2.f * PI) / FOCUS_CONFIG_ENCODER_CPR) *                                             \
             (((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR)) -                                   \
            (2.f * PI)))

#define ENCODER_TO_ELEC(count) (wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * ENCODER_TO_MECH(count)))

#define MECH_TO_ENCODER(theta)                                                                     \
    ((uint32_t)(((wrap(theta)) >= 0.f)                                                             \
                    ? ((FOCUS_CONFIG_ENCODER_CPR / (2.f * PI)) * wrap(theta))                      \
                    : (((FOCUS_CONFIG_ENCODER_CPR / (2.f * PI)) * wrap(theta)) +                   \
                       FOCUS_CONFIG_ENCODER_CPR)))

typedef struct {
    focus_context_t *context;
    focus_pid_t pid_d;
    focus_pid_t pid_q;
    focus_state_t state;
    focus_state_t requested_state;
    focus_callback_t requested_state_complete;

    float open_loop;
    int16_t encoder_lut[FOCUS_CONFIG_ENCODER_CPR];
    uint32_t encoder_lut_prev;

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

static uint16_t encoder_linear_interpolation(const int32_t x1,
                                             const int32_t y1,
                                             const int32_t x2,
                                             const int32_t y2,
                                             const int32_t xi) {
    if(xi == x1) {
        return y1;
    }

    if(xi == x2) {
        return y2;
    }

    return y1 + (((y2 - y1) * (xi - x1)) / (x2 - x1));
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

/*static void inverse_clark_transform(const float u_ab[2], float u_uvw[3]) {
    u_uvw[0] = u_ab[0];
    u_uvw[1] = -(0.5f * u_ab[0]) + (0.866025404f * u_ab[1]);
    u_uvw[2] = -(0.5f * u_ab[0]) - (0.866025404f * u_ab[1]);
}*/

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
    core.current_scale[0] = 1.f; // TODO
    core.current_scale[1] = 1.f; // TODO
    core.current_scale[2] = 1.f; // TODO
    memset(core.encoder_lut, 0, sizeof(core.encoder_lut));

    const float Rs = 0.13144f;  // TODO
    const float Ld = 0.000135f; // TODO
    const float Lq = 0.000135f; // TODO

    const float f = 10.f;
    const float w = 2.f * PI * f;

    const float Kpd = w * Ld;
    const float Kpq = w * Lq;
    const float Ki = w * Rs;

    // const float Kpd = 1.f;
    // const float Kpq = 1.f;
    // const float Ki = 0.1f;

    focus_pid_set_kp(&core.pid_d, Kpd);
    focus_pid_set_ki(&core.pid_d, Ki);
    focus_pid_set_kd(&core.pid_d, 0);
    focus_pid_antiwindup_enable(&core.pid_d, false);

    focus_pid_set_kp(&core.pid_q, Kpq);
    focus_pid_set_ki(&core.pid_q, Ki);
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
                    core.time_calibrate = time;
                    core.open_loop = 0;
                    core.encoder_lut_prev = 0;
                    memset(core.encoder_lut, 0, sizeof(core.encoder_lut));
                    core.state = FOCUS_STATE_CALIBRATE_ENCODER_INDEX;
                } break;
                case FOCUS_STATE_CLOSE_LOOP: {
                    focus_pid_start(&core.pid_d);
                    focus_pid_start(&core.pid_q);
                    focus_port_start(core.user);
                    core.state = FOCUS_STATE_CLOSE_LOOP;
                } break;
                case FOCUS_STATE_OPEN_LOOP: {
                    core.open_loop = 0;
                    focus_port_start(core.user);
                    core.state = FOCUS_STATE_OPEN_LOOP;
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
                core.open_loop = 0;
                core.state = FOCUS_STATE_CALIBRATE_ENCODER_ZERO;
            }
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY: {
            if((time - core.time_calibrate) >
               (0.5f * ((2.f * PI) / FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY_ECCENTRICITY))) {
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

    const float i_uvw[3] = {
        core.current_scale[0] * (sample->current_u - core.current_offset[0]),
        core.current_scale[1] * (sample->current_v - core.current_offset[1]),
        core.current_scale[2] * (sample->current_w - core.current_offset[2]),
    };

    switch(core.state) {
        case FOCUS_STATE_CALIBRATE_CURRENT: {
            core.current_offset[0] =
                ((core.current_offset[0] * core.current_offset_num) + sample->current_u) /
                (core.current_offset_num + 1);
            core.current_offset[1] =
                ((core.current_offset[1] * core.current_offset_num) + sample->current_v) /
                (core.current_offset_num + 1);
            core.current_offset[2] =
                ((core.current_offset[2] * core.current_offset_num) + sample->current_w) /
                (core.current_offset_num + 1);

            core.current_offset_num++;

            if((time - core.time_calibrate) >= FOCUS_CONFIG_CALIBRATE_CURRENT_TIME) {
                call_complete(&core);
                focus_port_event_panic();
            }
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_INDEX: {
            core.open_loop = wrap(core.open_loop + (FOCUS_CONFIG_SAMPLE_PERIOD *
                                                    FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY_INDEX));

            const uint32_t count = MECH_TO_ENCODER(core.open_loop);
            const float theta = ENCODER_TO_ELEC(count);

            const float u_dq[2] = {FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0};
            float u_ab[2];
            inverse_park_transform(u_dq, theta, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->vbus, duty_cycle_uvw);

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
            svpwm(u_ab, sample->vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);

            if((time - core.time_calibrate) >= FOCUS_CONFIG_CALIBRATE_ENCODER_TIME) {
                core.open_loop = 0;
                core.time_calibrate = time;
                core.state = FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY;
            }
        } break;
        case FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY: {
            core.open_loop =
                wrap(core.open_loop + (FOCUS_CONFIG_SAMPLE_PERIOD *
                                       FOCUS_CONFIG_CALIBRATE_ENCODER_VELOCITY_ECCENTRICITY));

            const uint32_t count = MECH_TO_ENCODER(core.open_loop);
            const float theta = ENCODER_TO_ELEC(count);

            const float u_dq[2] = {FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0};
            float u_ab[2];
            inverse_park_transform(u_dq, theta, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);

            const uint32_t enc_prev = core.encoder_lut_prev;
            const uint32_t enc_curr = sample->encoder % FOCUS_CONFIG_ENCODER_CPR;

            const int32_t diff_prev = core.encoder_lut[enc_prev];
            const int32_t diff_curr = ((int32_t)enc_curr) - ((int32_t)count);

            for(uint32_t i = enc_prev; i <= enc_curr; i++) {
                core.encoder_lut[i] =
                    encoder_linear_interpolation(enc_prev, diff_prev, enc_curr, diff_curr, i);
            }

            core.encoder_lut_prev = enc_curr;
        } break;
        case FOCUS_STATE_CLOSE_LOOP: {
            const float theta =
                ENCODER_TO_ELEC(sample->encoder - core.encoder_lut[sample->encoder]);

            float i_ab[2];
            clark_transform(i_uvw, i_ab);
            float i_dq[2];
            park_transform(i_ab, theta, i_dq);
            const float u_dq[2] = {
                focus_pid_calculate(&core.pid_d, 0.f, i_dq[0], FOCUS_CONFIG_SAMPLE_PERIOD),
                focus_pid_calculate(&core.pid_q, 1.f, i_dq[1], FOCUS_CONFIG_SAMPLE_PERIOD),
            };
            float u_ab[2];
            inverse_park_transform(u_dq, theta, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);

            if(scope_index < 1000) {
                scope_buffer[scope_index][0] = i_dq[0];
                scope_buffer[scope_index][1] = i_dq[1];
                scope_buffer[scope_index][2] =
                    ENCODER_TO_MECH(sample->encoder - core.encoder_lut[sample->encoder]);
                scope_index++;
            }
        } break;
        case FOCUS_STATE_OPEN_LOOP: {
            const float velocity = 2.f;
            const float voltage = 2.f;

            core.open_loop = wrap(core.open_loop + (FOCUS_CONFIG_SAMPLE_PERIOD * velocity));

            const uint32_t count = MECH_TO_ENCODER(core.open_loop);
            const float theta = ENCODER_TO_ELEC(count);

            const float u_dq[2] = {voltage, voltage / 2};
            float u_ab[2];
            inverse_park_transform(u_dq, theta, u_ab);
            float duty_cycle_uvw[3];
            svpwm(u_ab, sample->vbus, duty_cycle_uvw);

            const focus_port_control_t control = {
                .duty_cycle_u = duty_cycle_uvw[0],
                .duty_cycle_v = duty_cycle_uvw[1],
                .duty_cycle_w = duty_cycle_uvw[2],
            };
            focus_port_control(&control, core.user);

            if(scope_index < 1000) {
                scope_buffer[scope_index][0] = ((float)sample->encoder) - ((float)count);
                scope_buffer[scope_index][1] = ((float)sample->encoder) - ((float)count) -
                                               ((float)core.encoder_lut[sample->encoder]);
                scope_buffer[scope_index][2] = core.encoder_lut[sample->encoder];
                scope_index++;
            }

            /*{
                float i_ab[2];
                clark_transform(i_uvw, i_ab);
                float i_dq[2];
                park_transform(i_ab, theta, i_dq);
                if(scope_index < 1000) {
                    scope_buffer[scope_index][0] = i_dq[1];
                }
                {
                    const float theta = ENCODER_TO_ELEC(sample->encoder);
                    float i_ab[2];
                    clark_transform(i_uvw, i_ab);
                    float i_dq[2];
                    park_transform(i_ab, theta, i_dq);
                    if(scope_index < 1000) {
                        scope_buffer[scope_index][1] = i_dq[1];
                        scope_index++;
                    }
                }
            }*/
        } break;
        default: {

        } break;
    }

    core.context->uvw[0] = i_uvw[0];
    core.context->uvw[1] = i_uvw[1];
    core.context->uvw[2] = i_uvw[2];

    core.context->supply = sample->vbus;
    core.context->position = ENCODER_TO_MECH(sample->encoder - core.encoder_lut[sample->encoder]);
    core.context->position_open_loop = core.open_loop;
}

void focus_port_event_panic() {
    focus_port_shutdown(core.user);

    core.state = FOCUS_STATE_PANIC;
    core.time_panic = focus_port_timebase(core.user);
}
