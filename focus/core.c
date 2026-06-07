#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "focus/api.h"
#include "focus/biquad.h"
#include "focus/config.h"
#include "focus/debug.h"
#include "focus/fsm.h"
#include "focus/math.h"
#include "focus/pid.h"
#include "focus/port.h"
#include "focus/smo.h"

#define FOCUS_REQUESTED_STATE_NONE  0
#define FOCUS_REQUESTED_STATE_PANIC 1

#define FOCUS_FSM_TRANSITIONS_NUM 32

#define FOCUS_TORQUE_TO_CURRENT(torque) ((FOCUS_PI * FOCUS_CONFIG_MOTOR_KV * (torque)) / 30.f)

#define FOCUS_CURRENT_CALIBRATED(measurement, core, phase)                                         \
    ((core)->calibration.data.current.scale[(phase)] *                                             \
     ((measurement) - (core)->calibration.data.current.offset[(phase)]))

#define FOCUS_MECHANICAL_TO_ELECTRICAL(mech)                                                       \
    (focus_math_angle_wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS_NUM * (mech)))

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
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

#define FOCUS_ENCODER_ALIGNED(count, core)                                                         \
    ((((((uint32_t)(count)) + FOCUS_CONFIG_ENCODER_CPR) -                                          \
       (core)->calibration.data.encoder.align_offset)) %                                           \
     FOCUS_CONFIG_ENCODER_CPR)

#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
#define FOCUS_ENCODER_CALIBRATED(count, core)                                                      \
    (((FOCUS_ENCODER_ALIGNED((count), (core)) + FOCUS_CONFIG_ENCODER_CPR) -                        \
      (core)->calibration.data.encoder.eccentricity_lookup_table[FOCUS_ENCODER_ALIGNED((count),    \
                                                                                       (core))]) % \
     FOCUS_CONFIG_ENCODER_CPR)
#else
#define FOCUS_ENCODER_CALIBRATED(count, core) FOCUS_ENCODER_ALIGNED((count), (core))
#endif
#endif

typedef enum {
    FOCUS_STATE_IDLE,
    FOCUS_STATE_CALIBRATION_CURRENT,
    FOCUS_STATE_CALIBRATION_MOTOR_RESISTANCE,
    FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_D,
    FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_Q,
#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#ifdef FOCUS_CONFIG_ENCODER_AB
    FOCUS_STATE_RUNNING_ALIGN,
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
    FOCUS_STATE_RUNNING_ECCENTRICITY,
#endif
    FOCUS_STATE_RUNNING,
#endif
#ifdef FOCUS_CONFIG_ENCODER_ABI
    FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
    FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
    FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
#endif
    FOCUS_STATE_RUNNING_INDEX_SEARCH,
    FOCUS_STATE_RUNNING,
#endif
#ifdef FOCUS_CONFIG_ENCODER_ABSOLUTE
    FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
    FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
#endif
    FOCUS_STATE_RUNNING,
#endif
#endif
#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
    FOCUS_STATE_RUNNING_ALIGN,
    FOCUS_STATE_RUNNING_RAMP,
    FOCUS_STATE_RUNNING,
#endif
    FOCUS_STATE_NUM,
} focus_state_t;

typedef struct {
    uint32_t index;
    void *user;

    focus_pid_t pid_d;
    focus_pid_t pid_q;

    float iq_setpoint;
#ifndef FOCUS_CONFIG_SENSORLESS_ENABLE
    volatile float position;
#endif
    volatile float velocity;

    float current_state_enter_time;
    volatile focus_port_sample_t sample;

    focus_requested_state_t requested_state;
    focus_fsm_t fsm;
    focus_fsm_state_t fsm_states[FOCUS_STATE_NUM];
    focus_fsm_transition_t fsm_transitions[FOCUS_FSM_TRANSITIONS_NUM];

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
    struct {
        volatile float position_prev;
        focus_biquad_t velocity_filter;
    } encoder;
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
    struct {
        volatile float ramp_open_loop;
        focus_smo_t smo;
    } sensorless;
#endif

    struct {
        union {
            struct {
                volatile uint32_t num;
                volatile float time;
                volatile float buffer_u[FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES];
                volatile float buffer_v[FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES];
                volatile float buffer_w[FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES];
            } current;

            struct {
                volatile uint32_t num;
                volatile float time;
                volatile float buffer[FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES];
            } motor;

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
            struct {
                volatile float open_loop;
                volatile uint32_t lut_prev;
#ifdef FOCUS_CONFIG_ENCODER_ABI
                volatile bool index_occurred;
#endif
            } encoder;
#endif
        } context;

        volatile focus_calibration_t data;
    } calibration;
} focus_core_t;

static bool requested_idle(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_IDLE);
}

static bool requested_calibrate_current(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CALIBRATE_CURRENT);
}

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
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

static void calibration_current_enter(void *user) {
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

    core->calibration.data.current.offset[0] = 0.f;
    core->calibration.data.current.offset[1] = 0.f;
    core->calibration.data.current.offset[2] = 0.f;

    core->calibration.data.current.scale[0] = 1.f;
    core->calibration.data.current.scale[1] = 1.f;
    core->calibration.data.current.scale[2] = 1.f;
}

static void calibration_current_execute(void *user) {
    focus_core_t *core = user;

    const float duration = FOCUS_2PI / FOCUS_CONFIG_CURRENT_CALIBRATION_VELOCITY;
    const float period = duration / FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES;

    if(((core->calibration.context.current.num * period) <
        core->calibration.context.current.time) &&
       (core->calibration.context.current.num < FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES)) {
        core->calibration.context.current.buffer_u[core->calibration.context.current.num] =
            core->sample.current_u;
        core->calibration.context.current.buffer_v[core->calibration.context.current.num] =
            core->sample.current_v;
        core->calibration.context.current.buffer_w[core->calibration.context.current.num] =
            core->sample.current_w;
        core->calibration.context.current.num++;
    }

    const float mechanical_open_loop = focus_math_angle_wrap(
        FOCUS_CONFIG_CURRENT_CALIBRATION_VELOCITY * core->calibration.context.current.time);

    const float theta = FOCUS_MECHANICAL_TO_ELECTRICAL(mechanical_open_loop);

    const float u_dq[2] = {
        FOCUS_CONFIG_CURRENT_CALIBRATION_VOLTAGE,
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

    core->calibration.context.current.time += FOCUS_CONFIG_SAMPLING_PERIOD;
}

static void calibration_current_exit(void *user) {
    focus_core_t *core = user;

    const float duration = FOCUS_2PI / FOCUS_CONFIG_CURRENT_CALIBRATION_VELOCITY;
    const float period = duration / FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES;
    const float frequency = FOCUS_CONFIG_MOTOR_POLE_PAIRS_NUM / duration;

    float u_amplitude;
    float u_bias;
    focus_math_dft((float *)core->calibration.context.current.buffer_u,
                   FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES, period, frequency, &u_amplitude, NULL,
                   &u_bias);

    float v_amplitude;
    float v_bias;
    focus_math_dft((float *)core->calibration.context.current.buffer_v,
                   FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES, period, frequency, &v_amplitude, NULL,
                   &v_bias);

    float w_amplitude;
    float w_bias;
    focus_math_dft((float *)core->calibration.context.current.buffer_w,
                   FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES, period, frequency, &w_amplitude, NULL,
                   &w_bias);

    core->calibration.data.current.offset[0] = u_bias;
    core->calibration.data.current.offset[1] = v_bias;
    core->calibration.data.current.offset[2] = w_bias;

    const float mean_amplitude = (u_amplitude + v_amplitude + w_amplitude) / 3.f;

    core->calibration.data.current.scale[0] = mean_amplitude / u_amplitude;
    core->calibration.data.current.scale[1] = mean_amplitude / v_amplitude;
    core->calibration.data.current.scale[2] = mean_amplitude / w_amplitude;

    focus_calibration_update(core->index);
}

static bool calibration_current_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    const float duration = FOCUS_2PI / FOCUS_CONFIG_CURRENT_CALIBRATION_VELOCITY;
    return ((now - core->current_state_enter_time) > duration);
}

static void calibration_motor_resistance_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.motor.num = 0;
    memset((float *)core->calibration.context.motor.buffer, 0,
           sizeof(core->calibration.context.motor.buffer));
}

static void calibration_motor_resistance_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        FOCUS_CURRENT_CALIBRATED(core->sample.current_u, core, 0),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_v, core, 1),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_w, core, 2),
    };

    const float theta = 0.f;

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);

    if(core->calibration.context.motor.num < FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES) {
        core->calibration.context.motor.buffer[core->calibration.context.motor.num] = i_dq[0];
        core->calibration.context.motor.num++;
    }

    const float u_dq[2] = {
        FOCUS_CONFIG_MOTOR_CALIBRATION_RESISTANCE_VOLTAGE,
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
        _focus_debug_buffer_index = 0;
    }

    FOCUS_DEBUG_BUFFER_APPEND(u_dq[0], i_dq[0], 0);
}

static void calibration_motor_resistance_exit(void *user) {
    focus_core_t *core = user;

    const float ud = FOCUS_CONFIG_MOTOR_CALIBRATION_RESISTANCE_VOLTAGE;

    float id = 0;
    for(uint32_t i = 0; i < core->calibration.context.motor.num; i++) {
        id += core->calibration.context.motor.buffer[i];
    }
    id /= core->calibration.context.motor.num;

    core->calibration.data.motor.rs = ud / id;

    focus_calibration_update(core->index);
}

static bool calibration_motor_resistance_ended(const void *user) {
    const focus_core_t *core = user;
    return (core->calibration.context.motor.num >= FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES);
}

static void calibration_motor_inductance_d_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.motor.num = 0;
    core->calibration.context.motor.time = 0;
    memset((float *)core->calibration.context.motor.buffer, 0,
           sizeof(core->calibration.context.motor.buffer));
}

static void calibration_motor_inductance_d_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        FOCUS_CURRENT_CALIBRATED(core->sample.current_u, core, 0),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_v, core, 1),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_w, core, 2),
    };

    const float theta = 0.f;

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);

    if(core->calibration.context.motor.num < FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES) {
        core->calibration.context.motor.buffer[core->calibration.context.motor.num] = i_dq[0];
        core->calibration.context.motor.num++;
    }

    const float ud_amplitude = FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_FREQUENCY;

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
        _focus_debug_buffer_index = 0;
    }

    FOCUS_DEBUG_BUFFER_APPEND(u_dq[0], i_dq[0], 0);

    core->calibration.context.motor.time += FOCUS_CONFIG_SAMPLING_PERIOD;
}

static void calibration_motor_inductance_d_exit(void *user) {
    focus_core_t *core = user;

    const float ud_amplitude = FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_FREQUENCY;

    float id_amplitude;
    float id_phase;
    focus_math_dft((float *)core->calibration.context.motor.buffer,
                   FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES, FOCUS_CONFIG_SAMPLING_PERIOD,
                   FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_FREQUENCY, &id_amplitude, &id_phase,
                   NULL);

    const float z = ud_amplitude / id_amplitude;

    core->calibration.data.motor.ld = z * sinf(fabs(id_phase)) / w;

    focus_calibration_update(core->index);
}

static bool calibration_motor_inductance_d_ended(const void *user) {
    const focus_core_t *core = user;
    return (core->calibration.context.motor.num >= FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES);
}

static void calibration_motor_inductance_q_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.motor.num = 0;
    core->calibration.context.motor.time = 0;
    memset((float *)core->calibration.context.motor.buffer, 0,
           sizeof(core->calibration.context.motor.buffer));
}

static void calibration_motor_inductance_q_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        FOCUS_CURRENT_CALIBRATED(core->sample.current_u, core, 0),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_v, core, 1),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_w, core, 2),
    };

    const float theta = 0.f;

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);

    if(core->calibration.context.motor.num < FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES) {
        core->calibration.context.motor.buffer[core->calibration.context.motor.num] = i_dq[1];
        core->calibration.context.motor.num++;
    }

    const float uq_amplitude = FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_FREQUENCY;

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
        _focus_debug_buffer_index = 0;
    }

    FOCUS_DEBUG_BUFFER_APPEND(u_dq[1], i_dq[1], 0);

    core->calibration.context.motor.time += FOCUS_CONFIG_SAMPLING_PERIOD;
}

static void calibration_motor_inductance_q_exit(void *user) {
    focus_core_t *core = user;

    const float uq_amplitude = FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_VOLTAGE;
    const float w = FOCUS_2PI * FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_FREQUENCY;

    float iq_amplitude;
    float iq_phase;
    focus_math_dft((float *)core->calibration.context.motor.buffer,
                   FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES, FOCUS_CONFIG_SAMPLING_PERIOD,
                   FOCUS_CONFIG_MOTOR_CALIBRATION_INDUCTANCE_FREQUENCY, &iq_amplitude, &iq_phase,
                   NULL);

    const float z = uq_amplitude / iq_amplitude;

    core->calibration.data.motor.lq = z * sinf(fabs(iq_phase)) / w;

    focus_calibration_update(core->index);
}

static bool calibration_motor_inductance_q_ended(const void *user) {
    const focus_core_t *core = user;
    return (core->calibration.context.motor.num >= FOCUS_CONFIG_MOTOR_CALIBRATION_SAMPLES);
}

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#ifdef FOCUS_CONFIG_ENCODER_ABI
static void encoder_index_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.encoder.open_loop = 0;
    core->calibration.context.encoder.index_occurred = false;
}

static void encoder_index_execute(void *user) {
    focus_core_t *core = user;

    core->calibration.context.encoder.open_loop +=
        (FOCUS_CONFIG_SAMPLING_PERIOD * FOCUS_CONFIG_ENCODER_CALIBRATION_INDEX_SEARCH_VELOCITY);
    core->calibration.context.encoder.open_loop =
        focus_math_angle_wrap(core->calibration.context.encoder.open_loop);

    const float theta = FOCUS_MECHANICAL_TO_ELECTRICAL(core->calibration.context.encoder.open_loop);

    const float u_dq[2] = {
        FOCUS_CONFIG_ENCODER_CALIBRATION_INDEX_SEARCH_VOLTAGE,
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

    const float now = focus_port_timebase(core->user);
    if(((now - core->current_state_enter_time) <
        (0.1f * (FOCUS_2PI / FOCUS_CONFIG_ENCODER_CALIBRATION_INDEX_SEARCH_VELOCITY)))) {
        core->calibration.context.encoder.index_occurred = false;
    }

    core->position =
        FOCUS_ENCODER_TO_MECHANICAL(FOCUS_ENCODER_CALIBRATED(core->sample.encoder_count, core));
}

static bool encoder_index_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return (core->calibration.context.encoder.index_occurred &&
            ((now - core->current_state_enter_time) >
             (0.1f * (FOCUS_2PI / FOCUS_CONFIG_ENCODER_CALIBRATION_INDEX_SEARCH_VELOCITY))));
}

static bool encoder_index_timeout(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) >
            (1.1f * (FOCUS_2PI / FOCUS_CONFIG_ENCODER_CALIBRATION_INDEX_SEARCH_VELOCITY)));
}
#endif

static void encoder_align_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.encoder.open_loop = 0;
}

static void encoder_align_execute(void *user) {
    focus_core_t *core = user;

    const float u_dq[2] = {
        FOCUS_CONFIG_ENCODER_CALIBRATION_ALIGN_VOLTAGE,
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

    core->calibration.data.encoder.align_offset = core->sample.encoder_count;

    core->position =
        FOCUS_ENCODER_TO_MECHANICAL(FOCUS_ENCODER_CALIBRATED(core->sample.encoder_count, core));
}

static bool encoder_align_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) > FOCUS_CONFIG_ENCODER_CALIBRATION_ALIGN_TIME);
}

#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
static void encoder_eccentricity_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.context.encoder.open_loop = 0;
#ifdef FOCUS_CONFIG_ENCODER_ABI
    core->calibration.context.encoder.index_occurred = false;
#endif
    core->calibration.context.encoder.lut_prev = 0;
    memset((int32_t *)core->calibration.data.encoder.eccentricity_lookup_table, 0,
           sizeof(core->calibration.data.encoder.eccentricity_lookup_table));
}

static void encoder_eccentricity_execute(void *user) {
    focus_core_t *core = user;

    core->calibration.context.encoder.open_loop +=
        (FOCUS_CONFIG_SAMPLING_PERIOD * FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_VELOCITY);
    core->calibration.context.encoder.open_loop =
        focus_math_angle_wrap(core->calibration.context.encoder.open_loop);

    const uint32_t count = FOCUS_MECHANICAL_TO_ENCODER(core->calibration.context.encoder.open_loop);
    const float theta = FOCUS_MECHANICAL_TO_ELECTRICAL(core->calibration.context.encoder.open_loop);

    const float u_dq[2] = {
        FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_VOLTAGE,
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

    const uint32_t enc_prev = core->calibration.context.encoder.lut_prev;
    const uint32_t enc_curr = core->sample.encoder_count % FOCUS_CONFIG_ENCODER_CPR;

    const int32_t diff_prev = core->calibration.data.encoder.eccentricity_lookup_table[enc_prev];
    const int32_t diff_curr = ((int32_t)enc_curr) - ((int32_t)count);

    for(uint32_t i = enc_prev; i <= enc_curr; i++) {
        core->calibration.data.encoder.eccentricity_lookup_table[i] =
            focus_math_lerp(enc_prev, diff_prev, enc_curr, diff_curr, i);
    }

    core->calibration.context.encoder.lut_prev = enc_curr;

#ifdef FOCUS_CONFIG_ENCODER_ABI
    const float now = focus_port_timebase(core->user);
    if(((now - core->current_state_enter_time) <
        (0.5f * (FOCUS_2PI / FOCUS_CONFIG_ENCODER_CALIBRATION_INDEX_SEARCH_VELOCITY)))) {
        core->calibration.context.encoder.index_occurred = false;
    }
#endif

    core->position =
        FOCUS_ENCODER_TO_MECHANICAL(FOCUS_ENCODER_CALIBRATED(core->sample.encoder_count, core));
}

static bool encoder_eccentricity_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return (core->calibration.context.encoder.index_occurred &&
            ((now - core->current_state_enter_time) >
             (0.5f * (FOCUS_2PI / FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_VELOCITY))));
}
#endif
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
static void running_align_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
}

static void running_align_execute(void *user) {
    focus_core_t *core = user;

    const float u_dq[2] = {
        FOCUS_CONFIG_SENSORLESS_ALIGN_VOLTAGE,
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
}

static bool running_align_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) > FOCUS_CONFIG_SENSORLESS_ALIGN_TIME);
}

static void running_ramp_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->sensorless.ramp_open_loop = 0;

    focus_smo_init(&core->sensorless.smo, core->calibration.data.motor.rs,
                   core->calibration.data.motor.ld, core->calibration.data.motor.lq);
}

static void running_ramp_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        FOCUS_CURRENT_CALIBRATED(core->sample.current_u, core, 0),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_v, core, 1),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_w, core, 2),
    };

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);

    const float theta_e = FOCUS_MECHANICAL_TO_ELECTRICAL(core->sensorless.ramp_open_loop);

    const float u_dq[2] = {
        FOCUS_CONFIG_SENSORLESS_RAMP_VOLTAGE,
        0,
    };
    float u_dq_clamped[2];
    focus_math_clamp_vector(u_dq, core->sample.voltage_vbus / FOCUS_SQRT3, u_dq_clamped);
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

    const float now = focus_port_timebase(core->user);
    const float elapsed = now - core->current_state_enter_time;
    const float velocity = focus_math_sign(core->iq_setpoint) *
                           FOCUS_CONFIG_SENSORLESS_RAMP_VELOCITY *
                           (1.f - expf(-FOCUS_CONFIG_SENSORLESS_RAMP_LAMBDA * elapsed));

    core->sensorless.ramp_open_loop += (FOCUS_2PI * velocity * FOCUS_CONFIG_SAMPLING_PERIOD);
    core->sensorless.ramp_open_loop = focus_math_angle_wrap(core->sensorless.ramp_open_loop);

    focus_smo_update(&core->sensorless.smo, u_ab, i_ab);

    core->velocity = FOCUS_SMO_GET_ELECTRICAL_VELOCITY(&core->sensorless.smo) /
                     FOCUS_CONFIG_MOTOR_POLE_PAIRS_NUM;
}

static bool running_ramp_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) > FOCUS_CONFIG_SENSORLESS_RAMP_TIME);
}
#endif

static void running_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->iq_setpoint = 0;

    focus_pid_start(&core->pid_d);
    focus_pid_start(&core->pid_q);

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
    core->velocity = 0;
    core->encoder.position_prev =
        FOCUS_ENCODER_TO_MECHANICAL(FOCUS_ENCODER_CALIBRATED(core->sample.encoder_count, core));

    focus_biquad_design_lowpass(&core->encoder.velocity_filter,
                                FOCUS_CONFIG_ENCODER_VELOCITY_BANDWIDTH,
                                FOCUS_CONFIG_SAMPLING_FREQUENCY);
    focus_biquad_start(&core->encoder.velocity_filter);
#endif
}

static void running_execute(void *user) {
    focus_core_t *core = user;

    const float i_uvw[3] = {
        FOCUS_CURRENT_CALIBRATED(core->sample.current_u, core, 0),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_v, core, 1),
        FOCUS_CURRENT_CALIBRATED(core->sample.current_w, core, 2),
    };

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
    const float theta_e =
        FOCUS_ENCODER_TO_ELECTRICAL(FOCUS_ENCODER_CALIBRATED(core->sample.encoder_count, core));
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
    const float theta_e = FOCUS_SMO_GET_ELECTRICAL_POSITION(&core->sensorless.smo);
#endif

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta_e, i_dq);

    const float u_dq[2] = {
        focus_pid_calculate(&core->pid_d, 0.f, i_dq[0], FOCUS_CONFIG_SAMPLING_PERIOD),
        focus_pid_calculate(&core->pid_q, core->iq_setpoint, i_dq[1], FOCUS_CONFIG_SAMPLING_PERIOD),
    };

    const float u_dq_length = sqrtf((u_dq[0] * u_dq[0]) + (u_dq[1] * u_dq[1]));
    const float u_dq_length_max = core->sample.voltage_vbus / FOCUS_SQRT3;

    if(u_dq_length > u_dq_length_max) {
        const float u_dq_length_overflow = u_dq_length - u_dq_length_max;
        focus_pid_antiwindup(&core->pid_d, u_dq_length_overflow, FOCUS_CONFIG_SAMPLING_PERIOD);
        focus_pid_antiwindup(&core->pid_q, u_dq_length_overflow, FOCUS_CONFIG_SAMPLING_PERIOD);
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
    const float position_curr =
        FOCUS_ENCODER_TO_MECHANICAL(FOCUS_ENCODER_CALIBRATED(core->sample.encoder_count, core));
    const float velocity_curr = focus_math_angle_sub(position_curr, core->encoder.position_prev) /
                                FOCUS_CONFIG_SAMPLING_PERIOD;

    core->position = position_curr;
    core->velocity = focus_biquad_update(&core->encoder.velocity_filter, velocity_curr);

    core->encoder.position_prev = position_curr;
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
    focus_smo_update(&core->sensorless.smo, u_ab, i_ab);

    core->velocity = FOCUS_SMO_GET_ELECTRICAL_VELOCITY(&core->sensorless.smo) /
                     FOCUS_CONFIG_MOTOR_POLE_PAIRS_NUM;
#endif

    FOCUS_DEBUG_BUFFER_APPEND(i_dq[0], i_dq[1], theta_e);
}

#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
static bool running_close_loop_low_velocity(const void *user) {
    const focus_core_t *core = user;
    const float omega_e = FOCUS_SMO_GET_ELECTRICAL_VELOCITY(&core->sensorless.smo);
    const float omega_m = omega_e / FOCUS_CONFIG_MOTOR_POLE_PAIRS_NUM;
    return (fabs(omega_m) < FOCUS_CONFIG_SENSORLESS_VELOCITY_MINIMAL);
}
#endif

static focus_core_t cores[FOCUS_CONFIG_MOTORS_NUM] = {0};

void focus_init(void *user) {
    for(uint32_t i = 0; i < FOCUS_CONFIG_MOTORS_NUM; i++) {
        cores[i].index = i;
        cores[i].user = user;

        cores[i].velocity = 0.f;

        cores[i].requested_state = FOCUS_REQUESTED_STATE_NONE;

        focus_fsm_init(&cores[i].fsm, cores[i].fsm_states, FOCUS_STATE_NUM,
                       cores[i].fsm_transitions, FOCUS_FSM_TRANSITIONS_NUM, &cores[i]);

        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_IDLE, NULL, NULL, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_CURRENT,
                            calibration_current_enter, calibration_current_execute,
                            calibration_current_exit);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_MOTOR_RESISTANCE,
                            calibration_motor_resistance_enter,
                            calibration_motor_resistance_execute,
                            calibration_motor_resistance_exit);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_D,
                            calibration_motor_inductance_d_enter,
                            calibration_motor_inductance_d_execute,
                            calibration_motor_inductance_d_exit);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_Q,
                            calibration_motor_inductance_q_enter,
                            calibration_motor_inductance_q_execute,
                            calibration_motor_inductance_q_exit);
#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#ifdef FOCUS_CONFIG_ENCODER_AB
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, encoder_align_enter,
                            encoder_align_execute, NULL);
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY,
                            encoder_eccentricity_enter, encoder_eccentricity_execute, NULL);
#endif
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING, running_enter, running_execute,
                            NULL);
#endif
#ifdef FOCUS_CONFIG_ENCODER_ABI
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
                            encoder_index_enter, encoder_index_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                            encoder_align_enter, encoder_align_execute, NULL);
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
                            encoder_eccentricity_enter, encoder_eccentricity_execute, NULL);
#endif
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING_INDEX_SEARCH, encoder_index_enter,
                            encoder_index_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING, running_enter, running_execute,
                            NULL);
#endif
#ifdef FOCUS_CONFIG_ENCODER_ABSOLUTE
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                            encoder_align_enter, encoder_align_execute, NULL);
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
                            encoder_eccentricity_enter, encoder_eccentricity_execute, NULL);
#endif
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING, running_enter, running_execute,
                            NULL);
#endif
#endif
#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, running_align_enter,
                            running_align_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING_RAMP, running_ramp_enter,
                            running_ramp_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_RUNNING, running_enter, running_execute,
                            NULL);
#endif

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_CALIBRATION_CURRENT,
                                 requested_calibrate_current, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_CURRENT, FOCUS_STATE_IDLE,
                                 calibration_current_ended, core_shutdown);

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE,
                                 FOCUS_STATE_CALIBRATION_MOTOR_RESISTANCE,
                                 requested_calibrate_motor, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_MOTOR_RESISTANCE,
                                 FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_D,
                                 calibration_motor_resistance_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_D,
                                 FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_Q,
                                 calibration_motor_inductance_d_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_MOTOR_INDUCTANCE_Q,
                                 FOCUS_STATE_IDLE, calibration_motor_inductance_q_ended,
                                 core_shutdown);

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#ifdef FOCUS_CONFIG_ENCODER_AB
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_RUNNING_ALIGN,
                                 requested_close_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN,
                                 FOCUS_STATE_RUNNING_ECCENTRICITY, encoder_align_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY,
                                 FOCUS_STATE_RUNNING, encoder_eccentricity_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
#else
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, FOCUS_STATE_RUNNING,
                                 encoder_align_ended, NULL);
#endif
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
#endif
#ifdef FOCUS_CONFIG_ENCODER_ABI
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE,
                                 FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
                                 requested_calibrate_encoder, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
                                 FOCUS_STATE_IDLE, encoder_index_timeout, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
                                 FOCUS_STATE_IDLE, requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_INDEX_SEARCH,
                                 FOCUS_STATE_CALIBRATION_ENCODER_ALIGN, encoder_index_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_IDLE, requested_idle, core_shutdown);
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY, encoder_align_ended,
                                 NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
                                 FOCUS_STATE_IDLE, encoder_eccentricity_ended, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ECCENTRICITY,
                                 FOCUS_STATE_IDLE, requested_idle, core_shutdown);
#else
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_IDLE, encoder_align_ended, core_shutdown);
#endif

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_RUNNING_INDEX_SEARCH,
                                 requested_close_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_INDEX_SEARCH, FOCUS_STATE_IDLE,
                                 encoder_index_timeout, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_INDEX_SEARCH, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_INDEX_SEARCH, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_INDEX_SEARCH,
                                 FOCUS_STATE_RUNNING, encoder_index_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
#endif
#ifdef FOCUS_CONFIG_ENCODER_ABSOLUTE
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE,
                                 FOCUS_STATE_CALIBRATION_ENCODER_ALIGN, requested_calibrate_encoder,
                                 core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_IDLE, requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_RUNNING_ECCENTRICITY, encoder_align_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY, FOCUS_STATE_IDLE,
                                 encoder_eccentricity_ended, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ECCENTRICITY, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
#else
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATION_ENCODER_ALIGN,
                                 FOCUS_STATE_IDLE, encoder_align_ended, core_shutdown);
#endif

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_RUNNING,
                                 requested_close_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
#endif
#endif

#ifdef FOCUS_CONFIG_SENSORLESS_ENABLE
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_RUNNING_ALIGN,
                                 requested_close_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_ALIGN, FOCUS_STATE_RUNNING_RAMP,
                                 running_align_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_RAMP, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_RAMP, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING_RAMP, FOCUS_STATE_RUNNING,
                                 running_ramp_ended, NULL);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_RUNNING, FOCUS_STATE_RUNNING_ALIGN,
                                 running_close_loop_low_velocity, NULL);
#endif

        cores[i].calibration.data.motor.rs = 1E-1f;
        cores[i].calibration.data.motor.ld = 1E-4f;
        cores[i].calibration.data.motor.lq = 1E-4f;

        cores[i].calibration.data.current.offset[0] = 0.f;
        cores[i].calibration.data.current.offset[1] = 0.f;
        cores[i].calibration.data.current.offset[2] = 0.f;
        cores[i].calibration.data.current.scale[0] = 1.f;
        cores[i].calibration.data.current.scale[1] = 1.f;
        cores[i].calibration.data.current.scale[2] = 1.f;

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#ifdef FOCUS_CONFIG_ENCODER_CALIBRATION_ECCENTRICITY_ENABLE
        memset((int32_t *)cores[i].calibration.data.encoder.eccentricity_lookup_table, 0,
               sizeof(cores[i].calibration.data.encoder.eccentricity_lookup_table));
#endif
#endif

        focus_calibration_update(i);
    }

    focus_port_init(user);

    for(uint32_t i = 0; i < FOCUS_CONFIG_MOTORS_NUM; i++) {
        focus_fsm_start(&cores[i].fsm, FOCUS_STATE_IDLE);
    }
}

void focus_task() {
    for(uint32_t i = 0; i < FOCUS_CONFIG_MOTORS_NUM; i++) {
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
    cores[motor].iq_setpoint = FOCUS_TORQUE_TO_CURRENT(torque);
}

#ifndef FOCUS_CONFIG_SENSORLESS_ENABLE
float focus_get_position(const uint32_t motor) {
    return cores[motor].position;
}
#endif

float focus_get_velocity(const uint32_t motor) {
    return cores[motor].velocity;
}

#if (defined(FOCUS_CONFIG_ENCODER_ENABLE) && defined(FOCUS_CONFIG_ENCODER_ABI))
void focus_port_event_index(const uint32_t motor, const uint32_t encoder_count) {
    (void)encoder_count;

    cores[motor].calibration.context.encoder.index_occurred = true;
}
#endif

void focus_port_event_sample(const uint32_t motor, const focus_port_sample_t *sample) {
    cores[motor].sample = *sample;

    focus_fsm_execute(&cores[motor].fsm);

    _focus_debug_uvw[0] = FOCUS_CURRENT_CALIBRATED(sample->current_u, &cores[0], 0);
    _focus_debug_uvw[1] = FOCUS_CURRENT_CALIBRATED(sample->current_v, &cores[0], 1);
    _focus_debug_uvw[2] = FOCUS_CURRENT_CALIBRATED(sample->current_w, &cores[0], 2);

    _focus_debug_supply = sample->voltage_vbus;
#ifdef FOCUS_CONFIG_ENCODER_ABI
    _focus_debug_position_ol = cores[0].calibration.context.encoder.open_loop;
#endif
}

void focus_port_event_panic(const uint32_t motor) {
    focus_port_shutdown(motor, cores[motor].user);

    cores[motor].requested_state = FOCUS_REQUESTED_STATE_PANIC;
}
