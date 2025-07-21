#include "control/current_loop.h"

static void panic(current_loop_t *cl) {
    inverter_deinit(cl->inverter);
    encoder_deinit(cl->encoder);
}

static void control_loop(current_loop_t *cl) {
    (void)cl;

    // inverse clark/park transform
    // 2 PI controllers
    // clark/park transform
    // Space Vector Modulation

    inverter_set_duty_cycle(cl->inverter, 0, 0, 0);
}

static void inverter_handler(const inverter_event_t event, void *context) {
    current_loop_t *cl = context;

    (void)cl;

    switch(event) {
        case INVERTER_EVENT_ERROR: {
            panic(cl);
        } break;
        case INVERTER_EVENT_SAMPLE_START: {
            cl->current_ready = false;
            cl->position_ready = false;

            encoder_sample_start(cl->encoder);
        } break;
        case INVERTER_EVENT_SAMPLE_READY: {
            const status_t status = inverter_get_current(cl->inverter, &cl->current[0],
                                                         &cl->current[1], &cl->current[2]);
            if(status != STATUS_OK) {
                panic(cl);
                return;
            }

            cl->current_ready = true;

            if(cl->position_ready) {
                control_loop(cl);
            }
        } break;
    }
}

static void encoder_handler(const encoder_event_t event, void *context) {
    current_loop_t *cl = context;

    (void)cl;

    switch(event) {
        case ENCODER_EVENT_ERROR: {
            panic(cl);
        } break;
        case ENCODER_EVENT_SAMPLE_READY: {
            const status_t status = encoder_sample_get(cl->encoder, &cl->position);
            if(status != STATUS_OK) {
                panic(cl);
                return;
            }

            cl->position_ready = true;

            if(cl->current_ready) {
                control_loop(cl);
            }

        } break;
    }
}

status_t current_loop_start(current_loop_t *cl) {
    inverter_set_handler(cl->inverter, inverter_handler, cl);
    encoder_set_handler(cl->encoder, encoder_handler, cl);

    STATUS_RETURN(inverter_init(cl->inverter));
    STATUS_RETURN(encoder_init(cl->encoder));

    return STATUS_OK;
}

status_t current_loop_set_current_max(current_loop_t *cl, const float current_max) {
    cl->current_max = current_max;
    return STATUS_OK;
}

status_t current_loop_set_current_setpoint(current_loop_t *cl, const float current_setpoint) {
    cl->iq_ref = current_setpoint;
    return STATUS_OK;
}
