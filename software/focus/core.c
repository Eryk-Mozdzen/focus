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

#define FOCUS_ENCODER_TO_MECH(count)                                                               \
    (focus_math_wrap((((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR) <=                           \
                     (FOCUS_CONFIG_ENCODER_CPR / 2))                                               \
         ? ((FOCUS_2PI / FOCUS_CONFIG_ENCODER_CPR) *                                               \
            (((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR))                                      \
         : (((FOCUS_2PI / FOCUS_CONFIG_ENCODER_CPR) *                                              \
             (((uint32_t)(count)) % FOCUS_CONFIG_ENCODER_CPR)) -                                   \
            FOCUS_2PI))

#define FOCUS_ENCODER_TO_ELEC(count)                                                               \
    (focus_math_wrap(FOCUS_CONFIG_MOTOR_POLE_PAIRS * FOCUS_ENCODER_TO_MECH(count)))

#define FOCUS_MECH_TO_ENCODER(theta)                                                               \
    (((uint32_t)(((focus_math_wrap(theta)) >= 0.f)                                                 \
                     ? ((FOCUS_CONFIG_ENCODER_CPR / FOCUS_2PI) * focus_math_wrap(theta))           \
                     : (((FOCUS_CONFIG_ENCODER_CPR / FOCUS_2PI) * focus_math_wrap(theta)) +        \
                        FOCUS_CONFIG_ENCODER_CPR))) %                                              \
     FOCUS_CONFIG_ENCODER_CPR)

typedef enum {
    FOCUS_STATE_IDLE,
    FOCUS_STATE_CALIBRATE_CURRENT,
    FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
    FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
    FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
    FOCUS_STATE_CLOSE_LOOP,
    FOCUS_STATE_OPEN_LOOP,
    FOCUS_STATE_NUM,
} focus_state_t;

typedef struct {
    uint32_t index;
    void *user;

    focus_pid_t pid_d;
    focus_pid_t pid_q;

    float current_state_enter_time;
    volatile focus_port_sample_t sample;

    focus_requested_state_t requested_state;
    focus_fsm_t fsm;
    focus_fsm_state_t fsm_states[FOCUS_STATE_NUM];
    focus_fsm_transition_t fsm_transitions[FOCUS_FSM_TRANSITIONS_NUM];

    struct {
        volatile float open_loop;
        volatile bool index_occurred;
        volatile uint32_t encoder_lut_prev;
        volatile uint32_t current_offset_num;
        volatile focus_calibration_t data;
    } calibration;
} focus_core_t;

extern volatile float debug_supply;
extern volatile float debug_position;
extern volatile float debug_position_ol;
extern volatile float debug_svpwm[3];
extern volatile float debug_ab[2];
extern volatile float debug_uvw[3];
extern volatile float scope_buffer[1000][3];
extern volatile uint32_t scope_index;

static bool requested_idle(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_IDLE);
}

static bool requested_calibrate_current(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CALIBRATE_CURRENT);
}

static bool requested_calibrate_encoder(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CALIBRATE_ENCODER);
}

static bool requested_close_loop(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_CLOSE_LOOP);
}

static bool requested_open_loop(const void *user) {
    const focus_core_t *core = user;
    return (core->requested_state == FOCUS_REQUESTED_STATE_OPEN_LOOP);
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
    core->calibration.current_offset_num = 0;
    core->calibration.data.current_offset[0] = 0.f;
    core->calibration.data.current_offset[1] = 0.f;
    core->calibration.data.current_offset[2] = 0.f;
}

static void calibrate_current_execute(void *user) {
    focus_core_t *core = user;
    volatile focus_calibration_t *data = &core->calibration.data;
    const volatile focus_port_sample_t *sample = &core->sample;
    const uint32_t num = core->calibration.current_offset_num;

    data->current_offset[0] = ((data->current_offset[0] * num) + sample->current_u) / (num + 1);
    data->current_offset[1] = ((data->current_offset[1] * num) + sample->current_v) / (num + 1);
    data->current_offset[2] = ((data->current_offset[2] * num) + sample->current_w) / (num + 1);

    core->calibration.current_offset_num++;
}

static bool calibrate_current_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) > FOCUS_CONFIG_CALIBRATE_CURRENT_TIME);
}

static void calibrate_encoder_index_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.index_occurred = false;
    core->calibration.open_loop = 0;
    core->calibration.encoder_lut_prev = 0;
    memset((int32_t *)core->calibration.data.encoder_lut, 0,
           sizeof(core->calibration.data.encoder_lut));
}

static void calibrate_encoder_index_execute(void *user) {
    focus_core_t *core = user;

    core->calibration.open_loop = focus_math_wrap(
        core->calibration.open_loop +
        (FOCUS_CONFIG_SAMPLE_PERIOD * FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY));

    const uint32_t count = FOCUS_MECH_TO_ENCODER(core->calibration.open_loop);
    const float theta = FOCUS_ENCODER_TO_ELEC(count);

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
        core->calibration.index_occurred = false;
    }
}

static bool calibrate_encoder_index_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return (core->calibration.index_occurred &&
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
    core->calibration.open_loop = 0;
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
}

static bool calibrate_encoder_zero_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return ((now - core->current_state_enter_time) > FOCUS_CONFIG_CALIBRATE_ENCODER_ZERO_TIME);
}

static void calibrate_encoder_eccentricity_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
    core->calibration.open_loop = 0;
    core->calibration.index_occurred = false;
}

static void calibrate_encoder_eccentricity_execute(void *user) {
    focus_core_t *core = user;

    core->calibration.open_loop = focus_math_wrap(
        core->calibration.open_loop +
        (FOCUS_CONFIG_SAMPLE_PERIOD * FOCUS_CONFIG_CALIBRATE_ENCODER_ECCENTRICITY_VELOCITY));

    const uint32_t count = FOCUS_MECH_TO_ENCODER(core->calibration.open_loop);
    const float theta = FOCUS_ENCODER_TO_ELEC(count);

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

    const uint32_t enc_prev = core->calibration.encoder_lut_prev;
    const uint32_t enc_curr = core->sample.encoder_count % FOCUS_CONFIG_ENCODER_CPR;

    const int32_t diff_prev = core->calibration.data.encoder_lut[enc_prev];
    const int32_t diff_curr = ((int32_t)enc_curr) - ((int32_t)count);

    for(uint32_t i = enc_prev; i <= enc_curr; i++) {
        core->calibration.data.encoder_lut[i] =
            focus_math_lerp(enc_prev, diff_prev, enc_curr, diff_curr, i);
    }

    core->calibration.encoder_lut_prev = enc_curr;

    const float now = focus_port_timebase(core->user);
    if(((now - core->current_state_enter_time) <
        (0.5f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_INDEX_VELOCITY)))) {
        core->calibration.index_occurred = false;
    }
}

static bool calibrate_encoder_eccentricity_ended(const void *user) {
    const focus_core_t *core = user;
    const float now = focus_port_timebase(core->user);
    return (core->calibration.index_occurred &&
            ((now - core->current_state_enter_time) >
             (0.5f * (FOCUS_2PI / FOCUS_CONFIG_CALIBRATE_ENCODER_ECCENTRICITY_VELOCITY))));
}

static void close_loop_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
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

    const float theta =
        FOCUS_ENCODER_TO_ELEC(((int32_t)core->sample.encoder_count) -
                              core->calibration.data.encoder_lut[core->sample.encoder_count]);

    float i_ab[2];
    focus_math_clark_transform(i_uvw, i_ab);
    float i_dq[2];
    focus_math_park_transform(i_ab, theta, i_dq);
    const float u_dq[2] = {
        focus_pid_calculate(&core->pid_d, 0.f, i_dq[0], FOCUS_CONFIG_SAMPLE_PERIOD),
        focus_pid_calculate(&core->pid_q, 1.f, i_dq[1], FOCUS_CONFIG_SAMPLE_PERIOD),
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

    if(scope_index < 1000) {
        scope_buffer[scope_index][0] = i_dq[0];
        scope_buffer[scope_index][1] = i_dq[1];
        scope_buffer[scope_index][2] = theta;
        scope_index++;
    }
}

static void open_loop_enter(void *user) {
    focus_core_t *core = user;
    core->current_state_enter_time = focus_port_timebase(core->user);
}

static void open_loop_execute(void *user) {
    focus_core_t *core = user;

    const float velocity = FOCUS_CONFIG_CALIBRATE_ENCODER_ECCENTRICITY_VELOCITY;
    const float voltage = FOCUS_CONFIG_CALIBRATE_ENCODER_VOLTAGE;

    core->calibration.open_loop =
        focus_math_wrap(core->calibration.open_loop + (FOCUS_CONFIG_SAMPLE_PERIOD * velocity));

    const uint32_t count = FOCUS_MECH_TO_ENCODER(core->calibration.open_loop);
    const float theta = FOCUS_ENCODER_TO_ELEC(count);

    const float u_dq[2] = {voltage, 0};
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

    /*if(scope_index < 1000) {
        scope_buffer[scope_index][0] = ((int32_t)sample->encoder);
        scope_buffer[scope_index][1] = ((int32_t)count);
        scope_buffer[scope_index][2] = ((int32_t)count);
        scope_index++;
    }*/

    // core.context->svpwm[0] = duty_cycle_uvw[0];
    // core.context->svpwm[1] = duty_cycle_uvw[1];
    // core.context->svpwm[2] = duty_cycle_uvw[2];

    if(scope_index < 1000) {
        scope_buffer[scope_index][0] = ((int32_t)core->sample.encoder_count) - ((int32_t)count);
        scope_buffer[scope_index][1] =
            ((int32_t)core->sample.encoder_count) - ((int32_t)count) -
            core->calibration.data.encoder_lut[core->sample.encoder_count];
        scope_buffer[scope_index][2] =
            core->calibration.data.encoder_lut[core->sample.encoder_count];
        scope_index++;
    }
}

static focus_core_t cores[FOCUS_NUMBER_OF_MOTORS] = {0};

void focus_init(void *user) {
    for(uint32_t i = 0; i < FOCUS_NUMBER_OF_MOTORS; i++) {
        cores[i].index = i;
        cores[i].user = user;

        cores[i].requested_state = FOCUS_REQUESTED_STATE_NONE;

        focus_fsm_init(&cores[i].fsm, cores[i].fsm_states, FOCUS_STATE_NUM,
                       cores[i].fsm_transitions, FOCUS_FSM_TRANSITIONS_NUM, &cores[i]);

        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_IDLE, NULL, NULL, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_CURRENT, calibrate_current_enter,
                            calibrate_current_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                            calibrate_encoder_index_enter, calibrate_encoder_index_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
                            calibrate_encoder_zero_enter, calibrate_encoder_zero_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
                            calibrate_encoder_eccentricity_enter,
                            calibrate_encoder_eccentricity_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_CLOSE_LOOP, close_loop_enter,
                            close_loop_execute, NULL);
        focus_fsm_add_state(&cores[i].fsm, FOCUS_STATE_OPEN_LOOP, open_loop_enter,
                            open_loop_execute, NULL);

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_CALIBRATE_CURRENT,
                                 requested_calibrate_current, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_CURRENT, FOCUS_STATE_IDLE,
                                 calibrate_current_ended, core_shutdown);
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
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_CLOSE_LOOP,
                                 requested_close_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CLOSE_LOOP, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_IDLE, FOCUS_STATE_OPEN_LOOP,
                                 requested_open_loop, core_start);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_OPEN_LOOP, FOCUS_STATE_IDLE,
                                 requested_idle, core_shutdown);

        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_CURRENT, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_INDEX,
                                 FOCUS_STATE_IDLE, calibrate_encoder_index_timeout, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ZERO,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CALIBRATE_ENCODER_ECCENTRICITY,
                                 FOCUS_STATE_IDLE, core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_CLOSE_LOOP, FOCUS_STATE_IDLE,
                                 core_panicked, core_shutdown);
        focus_fsm_add_transition(&cores[i].fsm, FOCUS_STATE_OPEN_LOOP, FOCUS_STATE_IDLE,
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

        memset((int32_t *)cores[i].calibration.data.encoder_lut, 0,
               sizeof(cores[i].calibration.data.encoder_lut));

        focus_calibration_update(i);
    }

    focus_port_init(user);

    for(uint32_t i = 0; i < FOCUS_NUMBER_OF_MOTORS; i++) {
        focus_fsm_start(&cores[i].fsm, FOCUS_STATE_IDLE);
    }
}

void focus_task() {
    for(uint32_t i = 0; i < FOCUS_NUMBER_OF_MOTORS; i++) {
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
    const float w = FOCUS_2PI * FOCUS_MOTOR_BANDWIDTH;
    const float Kpd = w * cores[motor].calibration.data.motor.ld;
    const float Kpq = w * cores[motor].calibration.data.motor.lq;
    const float Ki = w * cores[motor].calibration.data.motor.rs;

    focus_pid_set_kp(&cores[motor].pid_d, Kpd);
    focus_pid_set_ki(&cores[motor].pid_d, Ki);
    focus_pid_set_kd(&cores[motor].pid_d, 0);
    focus_pid_antiwindup_enable(&cores[motor].pid_d, false);

    focus_pid_set_kp(&cores[motor].pid_q, Kpq);
    focus_pid_set_ki(&cores[motor].pid_q, Ki);
    focus_pid_set_kd(&cores[motor].pid_q, 0);
    focus_pid_antiwindup_enable(&cores[motor].pid_q, false);
}

void focus_port_event_index(const uint32_t motor, const uint32_t encoder_count) {
    (void)encoder_count;

    cores[motor].calibration.index_occurred = true;
}

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
    debug_position =
        FOCUS_ENCODER_TO_MECH(((int32_t)sample->encoder_count) -
                              cores[0].calibration.data.encoder_lut[sample->encoder_count]);
    debug_position_ol = cores[0].calibration.open_loop;
}

void focus_port_event_panic(const uint32_t motor) {
    focus_port_shutdown(motor, cores[motor].user);

    cores[motor].requested_state = FOCUS_REQUESTED_STATE_PANIC;
}
