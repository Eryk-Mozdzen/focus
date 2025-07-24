#include <math.h>

#include "focus/current_loop.h"

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

static void space_vector_pwm(const float u_uvw[3], const float supply, float dc_uvw[3]) {
    const float u_min = min3(u_uvw[0], u_uvw[1], u_uvw[2]);
    const float u_max = max3(u_uvw[0], u_uvw[1], u_uvw[2]);

    const float center = (0.5f * supply) - (0.5f * (u_max + u_min));

    dc_uvw[0] = clamp((u_uvw[0] + center) / supply, 0, 1);
    dc_uvw[1] = clamp((u_uvw[1] + center) / supply, 0, 1);
    dc_uvw[2] = clamp((u_uvw[2] + center) / supply, 0, 1);
}

static status_t field_oriented_control(current_loop_t *cl) {
    if((cl->current[0] > cl->overcurrent) || (cl->current[1] > cl->overcurrent) ||
       (cl->current[2] > cl->overcurrent)) {
        return STATUS_MOTOR_OVERCURRENT;
    }

    if(cl->supply > cl->overvoltage) {
        return STATUS_MOTOR_OVERVOLTAGE;
    }

    if(cl->supply < cl->undervoltage) {
        return STATUS_MOTOR_UNDERVOLTAGE;
    }

    const float theta =
        fmodf(cl->motor_pole_pairs * (cl->mech_position - cl->mech_position_offset), 6.283185307f);

    float i_dq[2];
    clark_park_transform(cl->current, theta, i_dq);

    if((i_dq[0] > cl->overcurrent) || (i_dq[1] > cl->overcurrent)) {
        return STATUS_MOTOR_OVERCURRENT;
    }

    const float dt = 0.1; // TODO

    float u_dq[2];
    u_dq[0] = pid_calculate(&cl->pi_controller_d, 0, i_dq[0], dt);
    u_dq[1] = pid_calculate(&cl->pi_controller_q, cl->current_setpoint, i_dq[1], dt);

    if((u_dq[0] > cl->overvoltage) || (u_dq[1] > cl->overvoltage)) {
        return STATUS_MOTOR_UNDERVOLTAGE;
    }

    float u_uvw[3];
    inverse_park_clark_transform(u_dq, theta, u_uvw);

    if((u_uvw[0] > cl->overvoltage) || (u_uvw[1] > cl->overvoltage) ||
       (u_uvw[2] > cl->overvoltage)) {
        return STATUS_MOTOR_UNDERVOLTAGE;
    }

    float dc_uvw[3];
    space_vector_pwm(u_uvw, cl->supply, dc_uvw);

    return inverter_set_duty_cycle(cl->inverter, dc_uvw[0], dc_uvw[1], dc_uvw[2]);
}

static void panic(current_loop_t *cl, const status_t error) {
    (void)error;

    inverter_deinit(cl->inverter);
    encoder_deinit(cl->encoder);
}

static void inverter_handler(const inverter_event_t event, void *context) {
    current_loop_t *cl = context;
    status_t status;

    switch(event) {
        case INVERTER_EVENT_SAMPLE_START: {
            cl->current_ready = false;
            cl->position_ready = false;

            encoder_sample_start(cl->encoder);
        } break;
        case INVERTER_EVENT_SAMPLE_READY: {
            float current[3];
            status = inverter_get_current(cl->inverter, &current[0], &current[1], &current[2]);
            if(status != STATUS_OK) {
                panic(cl, status);
                return;
            }

            status = memory_reg_write_float(&cl->current[0], current[0]);
            if(status != STATUS_OK) {
                panic(cl, status);
                return;
            }

            status = memory_reg_write_float(&cl->current[1], current[1]);
            if(status != STATUS_OK) {
                panic(cl, status);
                return;
            }

            status = memory_reg_write_float(&cl->current[2], current[2]);
            if(status != STATUS_OK) {
                panic(cl, status);
                return;
            }

            cl->current_ready = true;

            if(cl->position_ready) {
                status = field_oriented_control(cl);
                if(status != STATUS_OK) {
                    panic(cl, status);
                    return;
                }
            }
        } break;
    }
}

static void encoder_handler(const encoder_event_t event, void *context) {
    current_loop_t *cl = context;
    status_t status;

    switch(event) {
        case ENCODER_EVENT_SAMPLE_READY: {
            float position;
            status = encoder_sample_get(cl->encoder, &position);
            if(status != STATUS_OK) {
                panic(cl, status);
                return;
            }

            status = memory_reg_write_float(&cl->position, position);
            if(status != STATUS_OK) {
                panic(cl, status);
                return;
            }

            cl->position_ready = true;

            if(cl->current_ready) {
                status = field_oriented_control(cl);
                if(status != STATUS_OK) {
                    panic(cl, status);
                    return;
                }
            }

        } break;
    }
}

status_t start(current_loop_t *cl) {
    memory_reg_write_float(&cl->current_setpoint, 0);

    inverter_set_handler(cl->inverter, inverter_handler, cl);
    encoder_set_handler(cl->encoder, encoder_handler, cl);

    STATUS_RETURN(inverter_init(cl->inverter));
    STATUS_RETURN(encoder_init(cl->encoder));

    pid_start(&cl->pi_controller_d);
    pid_start(&cl->pi_controller_q);

    return STATUS_OK;
}
