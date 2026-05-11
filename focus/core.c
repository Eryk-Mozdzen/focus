#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "focus/api.h"
#include "focus/config.h"
#include "focus/fsm.h"
#include "focus/math.h"
#include "focus/pid.h"
#include "focus/port.h"

#define FOCUS_REQUESTED_STATE_NONE  0
#define FOCUS_REQUESTED_STATE_PANIC 1

#define FOCUS_FSM_TRANSITIONS_NUM 32

#define FOCUS_MECHANICAL_TO_ELECTRICAL(mech)                                                       \
    (focus_math_angle_wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * (mech)))

#ifdef FOCUS_CONFIG_ENCODER_ABI
#define FOCUS_ENCODER_TO_MECHANICAL(count)                                                         \
    (focus_math_angle_wrap((((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR) <=                     \
                           (FOCUS_CONFIG_ENCODER_CPR / 2))                                         \
         ? ((FOCUS_2PI / FOCUS_CONFIG_ENCODER_CPR) *                                               \
            (((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR))                                      \
         : (((FOCUS_2PI / FOCUS_CONFIG_ENCODER_CPR) *                                              \
             (((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR)) -                                   \
            FOCUS_2PI))

#define FOCUS_ENCODER_TO_ELECTRICAL(count)                                                         \
    (FOCUS_MECHANICAL_TO_ELECTRICAL(FOCUS_ENCODER_TO_MECHANICAL(count)))

#define FOCUS_MECHANICAL_TO_ENCODER(theta)                                                         \
    (((uint32_t)(((focus_math_angle_wrap(theta)) >= 0.f)                                           \
                     ? ((FOCUS_CONFIG_ENCODER_CPR / FOCUS_2PI) * focus_math_angle_wrap(theta))     \
                     : (((FOCUS_CONFIG_ENCODER_CPR / FOCUS_2PI) * focus_math_angle_wrap(theta)) +  \
                        FOCUS_CONFIG_ENCODER_CPR))) %                                              \
     FOCUS_CONFIG_ENCODER_CPR)
#endif

typedef enum {
    FOCUS_STATE_IDLE,
    FOCUS_STATE_CALIBRATE_CURRENT,
#ifdef FOCUS_CONFIG_ENCODER_ABI
    FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
    FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
    FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
#endif
    FOCUS_STATE_CALIBRATE_MOTOR_RESISTANCE,
    FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_D,
    FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_Q,
    FOCUS_STATE_CLOSE_LOOP,
    FOCUS_STATE_NUM,
} focus_state_t;

typedef struct {
    uint32_t index;
    void *user;

    focus_pid_t pid_d;
    focus_pid_t pid_q;

    float iq_setpoint;
    volatile float position;
    volatile float velocity;

    float current_state_enter_time;
    volatile focus_port_sample_t sample;

    focus_requested_state_t requested_state;
    focus_fsm_t fsm;
    focus_fsm_state_t fsm_states[FOCUS_STATE_NUM];
    focus_fsm_transition_t fsm_transitions[FOCUS_FSM_TRANSITIONS_NUM];

#ifdef FOCUS_CONFIG_ENCODER_ABI
    struct {
        volatile float position_prev;
    } encoder;
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_SMO
    struct {
        volatile float a;
        volatile float b;
        volatile float eta;

        volatile float i_ab_estimate[2];
        volatile float e_ab_estimate[2];
        volatile float i_ab_residual_prev[2];

        volatile float theta_e;
        volatile float omega_e;
    } smo;
#endif

    struct {
        union {
            struct {
                volatile uint32_t num;
                volatile float time;
                volatile float buffer_u[FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES];
                volatile float buffer_v[FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES];
                volatile float buffer_w[FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES];
            } current;

            struct {
                volatile uint32_t num;
                volatile float time;
                volatile float buffer[FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES];
            } motor;

#ifdef FOCUS_CONFIG_ENCODER_ABI
            struct {
                volatile float open_loop;
                volatile bool index_occurred;
                volatile uint32_t lut_prev;
            } encoder;
#endif
        } context;

        volatile focus_calibration_t data;
    } calibration;
} focus_core_t;

extern volatile float debug_supply;
extern volatile float debug_position_ol;
extern volatile float debug_svpwm[3];
extern volatile float debug_uvw[3];
extern volatile float debug_buffer[1000][3];
extern volatile uint32_t debug_buffer_index;

static bool requested_idle(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_IDLE);
}

static bool requested_calibrate_current(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CALIBRATE_CURRENT);
}

#ifdef FOCUS_CONFIG_ENCODER_ABI
static bool requested_calibrate_encoder(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CALIBRATE_ENCODER);
}
#endif

static bool requested_calibrate_motor(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CALIBRATE_MOTOR);
}

static bool requested_close_loop(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CLOSE_LOOP);
}

static bool core_panicked(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_PANIC);
}

static void core_start(void *user) {
    focus_core_t *core = user;
    focus_port_start(core->index, core->user);
}

static void core_shutdown(void *user) {
    focus_core_t *core = user;
    focus_port_shutdown(core->index, core->user);
}

static void calibrate_current_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);

    core->calibration.context.current.time = 0;
    core->calibration.context.current.num = 0;
    memset((float *)core->calibration.context.current.buffer_u, 0,
           sizeof(core->calibration.context.current.buffer_u));
    memset((float *)core->calibration.context.current.buffer_v, 0,
           sizeof(core->calibration.context.current.buffer_v));
    memset((float *)core->calibration.context.current.buffer_w, 0,
           sizeof(core->calibration.context.current.buffer_w));

    core->calibration.data.current_offset[0] = 0.f;
    core->calibration.data.current_offset[1] = 0.f;
    core->calibration.data.current_offset[2] = 0.f;

    core->calibration.data.current_scale[0] = 1.f;
    core->calibration.data.current_scale[1] = 1.f;
    core->calibration.data.current_scale[2] = 1.f;
}

static void calibrate_current_execute(void *user) {
    focus_core_t *core = user;

    const float duration = FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_CURRENT_VELOCITY;
    const float period = duration / FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES;

    if(((core->calibration.context.current.num * period) <
        core->calibration.context.current.time) &&
       (core->calibration.context.current.num < FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES)) {
        core->calibration.context.current.buffer_u[core->calibration.context.current.num] =
            core->sample.current_u;
        core->calibration.context.current.buffer_v[core->calibration.context.current.num] =
            core->sample.current_v;
        core->calibration.context.current.buffer_w[core->calibration.context.current.num] =
            core->sample.current_w;
        core->calibration.context.current.num++;
    }

    const float mechanical_open_loop = focus_math_angle_wrap(
        FOCUS_CONFIG_CALIBRATE_CURRENT_VELOCITY * core->calibration.context.current.time);

    const float theta = FOCUS_MECHANICAL_TO_ELECTRICAL(mechanical_open_loop);

    const float u_dq[2] = {
        FOCUS_CONFIG_CALIBRATE_CURRENT_VOLTAGE,
        0,
    };
    float u_dq_clamped[2];
    focus_math_clamp_vector(u_dq, core->sample.voltage_vbus / FOCUS_SQRT3, u_dq_clamped);
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq_clamped, theta, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    core->calibration.context.current.time += FOCUS_CONFIG_SAMPLE_PERIOD;
}

static void calibrate_current_exit(void *user) {
    focus_core_t *core = user;

    const float duration = FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_CURRENT_VELOCITY;
    const float period = duration / FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES;
    const float frequency = FOCUS_CONFIG_MOTOR_POLE_PAIRS / duration;

    float u_amplitude;
    float u_bias;
    focus_math_single_frequency_dft((float *)core->calibration.context.current.buffer_u,
                                    FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES, period, frequency,
                                    &u_amplitude, NULL, &u_bias);

    float v_amplitude;
    float v_bias;
    focus_math_single_frequency_dft((float *)core->calibration.context.current.buffer_v,
                                    FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES, period, frequency,
                                    &v_amplitude, NULL, &v_bias);

    float w_amplitude;
    float w_bias;
    focus_math_single_frequency_dft((float *)core->calibration.context.current.buffer_w,
                                    FOCUS_CONFIG_CALIBRATE_CURRENT_SAMPLES, period, frequency,
                                    &w_amplitude, NULL, &w_bias);

    core->calibration.data.current_offset[0] = u_bias;
    core->calibration.data.current_offset[1] = v_bias;
    core->calibration.data.current_offset[2] = w_bias;

    const float mean_amplitude = (u_amplitude + v_amplitude + w_amplitude) / 3.f;

    core->calibration.data.current_scale[0] = mean_amplitude / u_amplitude;
    core->calibration.data.current_scale[1] = mean_amplitude / v_amplitude;
    core->calibration.data.current_scale[2] = mean_amplitude / w_amplitude;

    focus_calibration_update(core->index);
}

static bool calibrate_current_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    const float duration = FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_CURRENT_VELOCITY;
    return ((now - core->current_state_enter_time) > duration);
}

#ifdef FOCUS_CONFIG_ENCODER_ABI
static void calibrate_encoder_index_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.encoder.open_loop = 0;
    core->calibration.context.encoder.index_occurred = false;
    core->calibration.context.encoder.lut_prev = 0;
    memset((int32_t *)core->calibration.data.encoder_lut, 0,
           sizeof(core->calibration.data.encoder_lut));
}

static void calibrate_encoder_index_execute(void *user) {
    focus_core_t *core = user;

    core->calibration.context.encoder.open_loop = focus_math_angle_wrap(
        core->calibration.context.encoder.open_loop +
        (FOCUS_CONFIG_SAMPLE_PERIOD * FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY));

    const float theta = FOCUS_MECHANICAL_TO_ELECTRICAL(core->calibration.context.encoder.open_loop);

    const float u_dq[2] = {FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0};
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq, theta, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    const float now = focus_port_timebase(core->user);
    if(((now - core->current_state_enter_time) <
        (0.1f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY)))) {
        core->calibration.context.encoder.index_occurred = false;
    }

    core->position =
        FOCUS_ENCODER_TO_MECHANICAL(((int32_t)core->sample.encoder_count) -
                                    core->calibration.data.encoder_lut[core->sample.encoder_count]);
}

static bool calibrate_encoder_index_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return (core->calibration.context.encoder.index_occurred &&
            ((now - core->current_state_enter_time) >
             (0.1f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY))));
}

static bool calibrate_encoder_index_timeout(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) >
            (1.1f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY)));
}

static void calibrate_encoder_zero_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.encoder.open_loop = 0;
}

static void calibrate_encoder_zero_execute(void *user) {
    focus_core_t *core = user;

    const float u_dq[2] = {FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0};
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq, 0, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    core->position =
        FOCUS_ENCODER_TO_MECHANICAL(((int32_t)core->sample.encoder_count) -
                                    core->calibration.data.encoder_lut[core->sample.encoder_count]);
}

static bool calibrate_encoder_zero_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) > FOCUS_CONFIG_CALIBRATE_ENCODER_ZERO_TIME);
}

static void calibrate_encoder_eccentricity_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.encoder.open_loop = 0;
    core->calibration.context.encoder.index_occurred = false;
}

static void calibrate_encoder_eccentricity_execute(void *user) {
    focus_core_t *core = user;

    core->calibration.context.encoder.open_loop = focus_math_angle_wrap(
        core->calibration.context.encoder.open_loop +
        (FOCUS_CONFIG_SAMPLE_PERIOD * FOCUS_CONFIG_CALIBRATE_ENCODER_ECCENTRICITY_VELOCITY));

    const uint32_t count = FOCUS_MECHANICAL_TO_ENCODER(core->calibration.context.encoder.open_loop);
    const float theta = FOCUS_MECHANICAL_TO_ELECTRICAL(core->calibration.context.encoder.open_loop);

    const float u_dq[2] = {FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE, 0};
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq, theta, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    const uint32_t enc_prev = core->calibration.context.encoder.lut_prev;
    const uint32_t enc_curr = core->sample.encoder_count % FOCUS_CONFIG_ENCODER_CPR;

    const int32_t diff_prev = core->calibration.data.encoder_lut[enc_prev];
    const int32_t diff_curr = ((int32_t)enc_curr) - ((int32_t)count);

    for(uint32_t i = enc_prev; i <= enc_curr; i++) {
        core->calibration.data.encoder_lut[i] =
            focus_math_lerp(enc_prev, diff_prev, enc_curr, diff_curr, i);
    }

    core->calibration.context.encoder.lut_prev = enc_curr;

    const float now = focus_port_timebase(core->user);
    if(((now - core->current_state_enter_time) <
        (0.5f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY)))) {
        core->calibration.context.encoder.index_occurred = false;
    }

    core->position =
        FOCUS_ENCODER_TO_MECHANICAL(((int32_t)core->sample.encoder_count) -
                                    core->calibration.data.encoder_lut[core->sample.encoder_count]);
}

static bool calibrate_encoder_eccentricity_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return (core->calibration.context.encoder.index_occurred &&
            ((now - core->current_state_enter_time) >
             (0.5f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_ECCENTRICITY_VELOCITY))));
}
#endif

static void calibrate_motor_resistance_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.motor.num = 0;
    memset((float *)core->calibration.context.motor.buffer, 0,
           sizeof(core->calibration.context.motor.buffer));
}

static void calibrate_motor_resistance_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        core->calibration.data.current_scale[0] *
            (core->sample.current_u - core->calibration.data.current_offset[0]),
        core->calibration.data.current_scale[1] *
            (core->sample.current_v - core->calibration.data.current_offset[1]),
        core->calibration.data.current_scale[2] *
            (core->sample.current_w - core->calibration.data.current_offset[2]),
    };

    const float theta = 0.f;

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);

    if(core->calibration.context.motor.num < FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES) {
        core->calibration.context.motor.buffer[core->calibration.context.motor.num] = i_dq[0];
        core->calibration.context.motor.num++;
    }

    const float u_dq[2] = {
        FOCUS_CONFIG_CALIBRATE_MOTOR_RESISTANCE_VOLTAGE,
        0,
    };
    float u_dq_clamped[2];
    focus_math_clamp_vector(u_dq, core->sample.voltage_vbus / FOCUS_SQRT3, u_dq_clamped);
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq_clamped, 0, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    if(core->calibration.context.motor.num == 1) {
        debug_buffer_index = 0;
    }

    if(debug_buffer_index < 1000) {
        debug_buffer[debug_buffer_index][0] = u_dq[0];
        debug_buffer[debug_buffer_index][1] = i_dq[0];
        debug_buffer[debug_buffer_index][2] = 0;
        debug_buffer_index++;
    }
}

static void calibrate_motor_resistance_exit(void *user) {
    focus_core_t *core = user;

    const float ud = FOCUS_CONFIG_CALIBRATE_MOTOR_RESISTANCE_VOLTAGE;

    float id = 0;
    for(uint32_t i = 0; i < core->calibration.context.motor.num; i++) {
        id += core->calibration.context.motor.buffer[i];
    }
    id /= core->calibration.context.motor.num;

    core->calibration.data.motor.rs = ud / id;

    focus_calibration_update(core->index);
}

static bool calibrate_motor_resistance_ended(const void *user) {
    const focus_core_t *core = user;
    return (core->calibration.context.motor.num >= FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES);
}

static void calibrate_motor_inductance_d_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.motor.num = 0;
    core->calibration.context.motor.time = 0;
    memset((float *)core->calibration.context.motor.buffer, 0,
           sizeof(core->calibration.context.motor.buffer));
}

static void calibrate_motor_inductance_d_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        core->calibration.data.current_scale[0] *
            (core->sample.current_u - core->calibration.data.current_offset[0]),
        core->calibration.data.current_scale[1] *
            (core->sample.current_v - core->calibration.data.current_offset[1]),
        core->calibration.data.current_scale[2] *
            (core->sample.current_w - core->calibration.data.current_offset[2]),
    };

    const float theta = 0.f;

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);

    if(core->calibration.context.motor.num < FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES) {
        core->calibration.context.motor.buffer[core->calibration.context.motor.num] = i_dq[0];
        core->calibration.context.motor.num++;
    }

    const float ud_amplitude = FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_FREQUENCY;

    const float u_dq[2] = {
        ud_amplitude * sinf(w * core->calibration.context.motor.time),
        0,
    };
    float u_dq_clamped[2];
    focus_math_clamp_vector(u_dq, core->sample.voltage_vbus / FOCUS_SQRT3, u_dq_clamped);
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq_clamped, 0, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    if(core->calibration.context.motor.num == 1) {
        debug_buffer_index = 0;
    }

    if(debug_buffer_index < 1000) {
        debug_buffer[debug_buffer_index][0] = u_dq[0];
        debug_buffer[debug_buffer_index][1] = i_dq[0];
        debug_buffer[debug_buffer_index][2] = 0;
        debug_buffer_index++;
    }

    core->calibration.context.motor.time += FOCUS_CONFIG_SAMPLE_PERIOD;
}

static void calibrate_motor_inductance_d_exit(void *user) {
    focus_core_t *core = user;

    const float ud_amplitude = FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_FREQUENCY;

    float id_amplitude;
    float id_phase;
    focus_math_single_frequency_dft(
        (float *)core->calibration.context.motor.buffer, FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES,
        FOCUS_CONFIG_SAMPLE_PERIOD, FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_FREQUENCY,
        &id_amplitude, &id_phase, NULL);

    const float z = ud_amplitude / id_amplitude;

    core->calibration.data.motor.ld = z * sinf(fabs(id_phase)) / w;

    focus_calibration_update(core->index);
}

static bool calibrate_motor_inductance_d_ended(const void *user) {
    const focus_core_t *core = user;
    return (core->calibration.context.motor.num >= FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES);
}

static void calibrate_motor_inductance_q_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.motor.num = 0;
    core->calibration.context.motor.time = 0;
    memset((float *)core->calibration.context.motor.buffer, 0,
           sizeof(core->calibration.context.motor.buffer));
}

static void calibrate_motor_inductance_q_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        core->calibration.data.current_scale[0] *
            (core->sample.current_u - core->calibration.data.current_offset[0]),
        core->calibration.data.current_scale[1] *
            (core->sample.current_v - core->calibration.data.current_offset[1]),
        core->calibration.data.current_scale[2] *
            (core->sample.current_w - core->calibration.data.current_offset[2]),
    };

    const float theta = 0.f;

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);

    if(core->calibration.context.motor.num < FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES) {
        core->calibration.context.motor.buffer[core->calibration.context.motor.num] = i_dq[1];
        core->calibration.context.motor.num++;
    }

    const float uq_amplitude = FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_FREQUENCY;

    const float u_dq[2] = {
        0,
        uq_amplitude * sinf(w * core->calibration.context.motor.time),
    };
    float u_dq_clamped[2];
    focus_math_clamp_vector(u_dq, core->sample.voltage_vbus / FOCUS_SQRT3, u_dq_clamped);
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq_clamped, 0, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

    if(core->calibration.context.motor.num == 1) {
        debug_buffer_index = 0;
    }

    if(debug_buffer_index < 1000) {
        debug_buffer[debug_buffer_index][0] = u_dq[1];
        debug_buffer[debug_buffer_index][1] = i_dq[1];
        debug_buffer[debug_buffer_index][2] = 0;
        debug_buffer_index++;
    }

    core->calibration.context.motor.time += FOCUS_CONFIG_SAMPLE_PERIOD;
}

static void calibrate_motor_inductance_q_exit(void *user) {
    focus_core_t *core = user;

    const float uq_amplitude = FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_FREQUENCY;

    float iq_amplitude;
    float iq_phase;
    focus_math_single_frequency_dft(
        (float *)core->calibration.context.motor.buffer, FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES,
        FOCUS_CONFIG_SAMPLE_PERIOD, FOCUS_CONFIG_CALIBRATE_MOTOR_INDUCTANCE_FREQUENCY,
        &iq_amplitude, &iq_phase, NULL);

    const float z = uq_amplitude / iq_amplitude;

    core->calibration.data.motor.lq = z * sinf(fabs(iq_phase)) / w;

    focus_calibration_update(core->index);
}

static bool calibrate_motor_inductance_q_ended(const void *user) {
    const focus_core_t *core = user;
    return (core->calibration.context.motor.num >= FOCUS_CONFIG_CALIBRATE_MOTOR_SAMPLES);
}

static void close_loop_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->iq_setpoint = 0;

    focus_pid_start(&core->pid_d);
    focus_pid_start(&core->pid_q);

#ifdef FOCUS_CONFIG_ENCODER_ABI
    const int32_t count_calibrated = ((int32_t)core->sample.encoder_count) -
                                     core->calibration.data.encoder_lut[core->sample.encoder_count];
    const float theta_m = FOCUS_ENCODER_TO_MECHANICAL(count_calibrated);

    core->velocity = 0;
    core->encoder.position_prev = theta_m;
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_SMO
    const float R = core->calibration.data.motor.rs;
    const float L = 0.5f * (core->calibration.data.motor.ld + core->calibration.data.motor.lq);

    core->smo.a = expf(-(R * FOCUS_CONFIG_SAMPLE_PERIOD) / L);
    core->smo.b = (1.f / R) * (1.f - expf(-(R * FOCUS_CONFIG_SAMPLE_PERIOD) / L));
    core->smo.eta =
        2.f * ((core->smo.b * FOCUS_CONFIG_SENSORLESS_SMO_M) / FOCUS_CONFIG_SENSORLESS_SMO_G);

    core->smo.i_ab_estimate[0] = 0.f;
    core->smo.i_ab_estimate[1] = 0.f;

    core->smo.e_ab_estimate[0] = 0.f;
    core->smo.e_ab_estimate[1] = 0.f;

    core->smo.i_ab_residual_prev[0] = 0.f;
    core->smo.i_ab_residual_prev[1] = 0.f;

    core->smo.theta_e = 0.f;
    core->smo.theta_e = 0.f;
#endif
}

static void close_loop_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        core->calibration.data.current_scale[0] *
            (core->sample.current_u - core->calibration.data.current_offset[0]),
        core->calibration.data.current_scale[1] *
            (core->sample.current_v - core->calibration.data.current_offset[1]),
        core->calibration.data.current_scale[2] *
            (core->sample.current_w - core->calibration.data.current_offset[2]),
    };

#ifdef FOCUS_CONFIG_ENCODER_ABI
    const int32_t count_calibrated = ((int32_t)core->sample.encoder_count) -
                                     core->calibration.data.encoder_lut[core->sample.encoder_count];
    const float theta_e = FOCUS_ENCODER_TO_ELECTRICAL(count_calibrated);
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_SMO
    const float theta_e = core->smo.theta_e;
#endif

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta_e, i_dq);

    const float u_dq[2] = {
        focus_pid_calculate(&core->pid_d, 0.f, i_dq[0], FOCUS_CONFIG_SAMPLE_PERIOD),
        focus_pid_calculate(&core->pid_q, core->iq_setpoint, i_dq[1], FOCUS_CONFIG_SAMPLE_PERIOD),
    };

    const float u_dq_length = sqrtf((u_dq[0] * u_dq[0]) + (u_dq[1] * u_dq[1]));
    const float u_dq_length_max = core->sample.voltage_vbus / FOCUS_SQRT3;

    if(u_dq_length > u_dq_length_max) {
        const float u_dq_length_overflow = u_dq_length - u_dq_length_max;
        focus_pid_antiwindup(&core->pid_d, u_dq_length_overflow, FOCUS_CONFIG_SAMPLE_PERIOD);
        focus_pid_antiwindup(&core->pid_q, u_dq_length_overflow, FOCUS_CONFIG_SAMPLE_PERIOD);
    }

    float u_dq_clamped[2];
    focus_math_clamp_vector(u_dq, u_dq_length_max, u_dq_clamped);
    float u_ab[2];
    focus_math_inverse_park_transform(u_dq_clamped, theta_e, u_ab);
    float duty_cycle_uvw[3];
    focus_math_svpwm(u_ab, core->sample.voltage_vbus, duty_cycle_uvw);

    const focus_port_control_t control = {
        .duty_cycle_u = duty_cycle_uvw[0],
        .duty_cycle_v = duty_cycle_uvw[1],
        .duty_cycle_w = duty_cycle_uvw[2],
    };
    focus_port_control(core->index, &control, core->user);

#ifdef FOCUS_CONFIG_ENCODER_ABI
    const float position_curr = FOCUS_ENCODER_TO_MECHANICAL(count_calibrated);
    const float velocity_curr = focus_math_angle_sub(position_curr, core->encoder.position_prev) /
                                FOCUS_CONFIG_SAMPLE_PERIOD;

    core->position = position_curr;
    core->velocity = (FOCUS_CONFIG_ENCODER_VELOCITY_FILTER * core->velocity) +
                     ((1.f - FOCUS_CONFIG_ENCODER_VELOCITY_FILTER) * velocity_curr);

    core->encoder.position_prev = position_curr;
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_SMO
    const float dir_prev = atan2f(core->smo.e_ab_estimate[1], core->smo.e_ab_estimate[0]);

    const float i_ab_residual[2] = {
        core->smo.i_ab_estimate[0] - i_ab[0],
        core->smo.i_ab_estimate[1] - i_ab[1],
    };

    core->smo.i_ab_estimate[0] = (core->smo.a * core->smo.i_ab_estimate[0]) +
                                 (core->smo.b * (u_ab[0] - core->smo.e_ab_estimate[0])) -
                                 (core->smo.eta * focus_math_sign(i_ab_residual[0]));
    core->smo.i_ab_estimate[1] = (core->smo.a * core->smo.i_ab_estimate[1]) +
                                 (core->smo.b * (u_ab[1] - core->smo.e_ab_estimate[1])) -
                                 (core->smo.eta * focus_math_sign(i_ab_residual[1]));

    core->smo.e_ab_estimate[0] =
        core->smo.e_ab_estimate[0] +
        ((FOCUS_CONFIG_SENSORLESS_SMO_G / core->smo.b) *
         (i_ab_residual[0] - (core->smo.a * core->smo.i_ab_residual_prev[0]) +
          (core->smo.eta * focus_math_sign(core->smo.i_ab_residual_prev[0]))));
    core->smo.e_ab_estimate[1] =
        core->smo.e_ab_estimate[1] +
        ((FOCUS_CONFIG_SENSORLESS_SMO_G / core->smo.b) *
         (i_ab_residual[1] - (core->smo.a * core->smo.i_ab_residual_prev[1]) +
          (core->smo.eta * focus_math_sign(core->smo.i_ab_residual_prev[1]))));

    core->smo.i_ab_residual_prev[0] = i_ab_residual[0];
    core->smo.i_ab_residual_prev[1] = i_ab_residual[1];

    const float dir_curr = atan2f(core->smo.e_ab_estimate[1], core->smo.e_ab_estimate[0]);
    const float omega_e = focus_math_angle_sub(dir_curr, dir_prev) / FOCUS_CONFIG_SAMPLE_PERIOD;

    core->smo.omega_e = (FOCUS_CONFIG_SENSORLESS_SMO_VELOCITY_FILTER * core->smo.omega_e) +
                        ((1.f - FOCUS_CONFIG_SENSORLESS_SMO_VELOCITY_FILTER) * omega_e);
    core->smo.theta_e =
        focus_math_angle_wrap(dir_curr - (focus_math_sign(core->smo.omega_e) * FOCUS_HALF_PI));

    core->position = core->smo.theta_e; // TODO
    core->velocity = core->smo.omega_e; // TODO  / FOCUS_CONFIG_MOTOR_POLE_PAIRS;
#endif

    if(debug_buffer_index < 1000) {
        debug_buffer[debug_buffer_index][0] = i_dq[0];
        debug_buffer[debug_buffer_index][1] = i_dq[1];
        debug_buffer[debug_buffer_index][2] = theta_e;
        debug_buffer_index++;
    }
}

static focus_core_t cores[FOCUS_CONFIG_NUMBER_OF_MOTORS] = {0};

void focus_init(void *user) {
    for(uint32_t i = 0; i < FOCUS_CONFIG_NUMBER_OF_MOTORS; i++) {
        cores[i].index = i;
        cores[i].user = user;

        cores[i].velocity = 0.f;

        cores[i].requested_state = FOCUS_REQUESTED_STATE_NONE;

        focus_fsm_init(&cores[i].fsm, cores[i].fsm_states, FOCUS_STATE_NUM,
                       cores[i].fsm_transitions, FOCUS_FSM_TRANSITIONS_NUM, &cores[i]);

        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_IDLE, NULL, NULL, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_CURRENT, calibrate_current_enter,
                            calibrate_current_execute, calibrate_current_exit);
#ifdef FOCUS_CONFIG_ENCODER_ABI
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                            calibrate_encoder_index_enter, calibrate_encoder_index_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
                            calibrate_encoder_zero_enter, calibrate_encoder_zero_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
                            calibrate_encoder_eccentricity_enter,
                            calibrate_encoder_eccentricity_execute, NULL);
#endif
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_RESISTANCE,
                            calibrate_motor_resistance_enter, calibrate_motor_resistance_execute,
                            calibrate_motor_resistance_exit);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_D,
                            calibrate_motor_inductance_d_enter,
                            calibrate_motor_inductance_d_execute,
                            calibrate_motor_inductance_d_exit);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_Q,
                            calibrate_motor_inductance_q_enter,
                            calibrate_motor_inductance_q_execute,
                            calibrate_motor_inductance_q_exit);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CLOSE_LOOP, close_loop_enter,
                            close_loop_execute, NULL);

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_CALIBRATE_CURRENT,
                                 requested_calibrate_current, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_CURRENT, FOCUS_STATE_IDLE,
                                 calibrate_current_ended, core_shutdown);
#ifdef FOCUS_CONFIG_ENCODER_ABI
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE,
                                 FOCUS_STATE_CALIBRATE_ENCODER_INDEX, requested_calibrate_encoder,
                                 core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                                 FOCUS_STATE_CALIBRATE_ENCODER_ZERO, calibrate_encoder_index_ended,
                                 NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
                                 FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
                                 calibrate_encoder_zero_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
                                 FOCUS_STATE_IDLE, calibrate_encoder_eccentricity_ended,
                                 core_shutdown);
#endif
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE,
                                 FOCUS_STATE_CALIBRATE_MOTOR_RESISTANCE, requested_calibrate_motor,
                                 core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_RESISTANCE,
                                 FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_D,
                                 calibrate_motor_resistance_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_D,
                                 FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_Q,
                                 calibrate_motor_inductance_d_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_Q,
                                 FOCUS_STATE_IDLE, calibrate_motor_inductance_q_ended,
                                 core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_CLOSE_LOOP,
                                 requested_close_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CLOSE_LOOP, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_CURRENT, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
#ifdef FOCUS_CONFIG_ENCODER_ABI
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                                 FOCUS_STATE_IDLE, calibrate_encoder_index_timeout, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
#endif
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_RESISTANCE,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_D,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_MOTOR_INDUCTANCE_Q,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CLOSE_LOOP, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);

        cores[i].calibration.data.motor.rs = 1E-1f;
        cores[i].calibration.data.motor.ld = 1E-4f;
        cores[i].calibration.data.motor.lq = 1E-4f;

        cores[i].calibration.data.current_offset[0] = 0.f;
        cores[i].calibration.data.current_offset[1] = 0.f;
        cores[i].calibration.data.current_offset[2] = 0.f;
        cores[i].calibration.data.current_scale[0] = 1.f;
        cores[i].calibration.data.current_scale[1] = 1.f;
        cores[i].calibration.data.current_scale[2] = 1.f;

#ifdef FOCUS_CONFIG_ENCODER_ABI
        memset((int32_t *)cores[i].calibration.data.encoder_lut, 0,
               sizeof(cores[i].calibration.data.encoder_lut));
#endif

        focus_calibration_update(i);
    }

    focus_port_init(user);

    for(uint32_t i = 0; i < FOCUS_CONFIG_NUMBER_OF_MOTORS; i++) {
        focus_fsm_start(&cores[i].fsm, FOCUS_STATE_IDLE);
    }
}

void focus_task() {
    for(uint32_t i = 0; i < FOCUS_CONFIG_NUMBER_OF_MOTORS; i++) {
        focus_fsm_update(&cores[i].fsm);

        cores[i].requested_state = FOCUS_REQUESTED_STATE_NONE;
    }
}

void focus_request_state(const uint32_t motor, const focus_requested_state_t requested_state) {
    cores[motor].requested_state = requested_state;
}

focus_calibration_t *focus_calibration_data(const uint32_t motor) {
    return (focus_calibration_t *)&cores[motor].calibration.data;
}

void focus_calibration_update(const uint32_t motor) {
    const float w = FOCUS_2PI * FOCUS_CONFIG_FOC_BANDWIDTH;
    const float Kpd = w * cores[motor].calibration.data.motor.ld;
    const float Kpq = w * cores[motor].calibration.data.motor.lq;
    const float Ki = w * cores[motor].calibration.data.motor.rs;

    focus_pid_set_kp(&cores[motor].pid_d, Kpd);
    focus_pid_set_ki(&cores[motor].pid_d, Ki);
    focus_pid_set_kd(&cores[motor].pid_d, 0.f);
    focus_pid_set_ka(&cores[motor].pid_d, 1.f);

    focus_pid_set_kp(&cores[motor].pid_q, Kpq);
    focus_pid_set_ki(&cores[motor].pid_q, Ki);
    focus_pid_set_kd(&cores[motor].pid_q, 0.f);
    focus_pid_set_ka(&cores[motor].pid_q, 1.f);
}

void focus_set_torque(const uint32_t motor, const float torque) {
    cores[motor].iq_setpoint = torque;
}

float focus_get_position(const uint32_t motor) {
    return cores[motor].position;
}

float focus_get_velocity(const uint32_t motor) {
    return cores[motor].velocity;
}

#ifdef FOCUS_CONFIG_ENCODER_ABI
void focus_port_event_index(const uint32_t motor, const uint32_t encoder_count) {
    (void)encoder_count;

    cores[motor].calibration.context.encoder.index_occurred = true;
}
#endif

void focus_port_event_sample(const uint32_t motor, const focus_port_sample_t *sample) {
    cores[motor].sample = *sample;

    focus_fsm_execute(&cores[motor].fsm);

    debug_uvw[0] = cores[0].calibration.data.current_scale[0] *
                   (sample->current_u - cores[0].calibration.data.current_offset[0]);
    debug_uvw[1] = cores[0].calibration.data.current_scale[1] *
                   (sample->current_v - cores[0].calibration.data.current_offset[1]);
    debug_uvw[2] = cores[0].calibration.data.current_scale[2] *
                   (sample->current_w - cores[0].calibration.data.current_offset[2]);

    debug_supply = sample->voltage_vbus;
#ifdef FOCUS_CONFIG_ENCODER_ABI
    debug_position_ol = cores[0].calibration.context.encoder.open_loop;
#endif
}

void focus_port_event_panic(const uint32_t motor) {
    focus_port_shutdown(motor, cores[motor].user);

    cores[motor].requested_state = FOCUS_REQUESTED_STATE_PANIC;
}
