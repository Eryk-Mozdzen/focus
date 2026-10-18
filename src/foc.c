#include <stdbool.h>
#include <stdint.h>

#include "focus/biquad.h"
#include "focus/common.h"
#include "focus/foc.h"
#include "focus/log.h"
#include "focus/math.h"
#include "focus/pid.h"

#define FOCUS_LOG_MODULE FOCUS_LOG_MODULE_SRV_CONTROL

void focus_foc_init(struct focus_foc *foc) {
    foc->srv.inverter->control = &foc->srv;
    foc->srv.position->control = &foc->srv;

    foc->state_current = FOCUS_FOC_STATE_IDLE;
    foc->state_requested = FOCUS_FOC_STATE_NONE;
}

void focus_foc_req_state(struct focus_foc *foc, enum focus_foc_state state) {
    foc->state_requested = state;

    FOCUS_LOG_DBG("state requested: %d", state);
}

void focus_foc_set_torque(struct focus_foc *foc, float torque) {
}

float focus_foc_get_position(struct focus_foc *foc) {
    return foc->position;
}

float focus_foc_get_velocity(struct focus_foc *foc) {
    return foc->velocity;
}

float focus_foc_get_voltage(struct focus_foc *foc) {
    return foc->voltage;
}

void focus_foc_task(struct focus_foc *foc) {
    switch(foc->state_current) {
        case FOCUS_FOC_STATE_NONE: {
            FOCUS_LOG_ERR("invalid state");
            foc->state_current = FOCUS_FOC_STATE_IDLE;
        } break;
        case FOCUS_FOC_STATE_IDLE: {
            switch(foc->state_requested) {
                case FOCUS_FOC_STATE_CALIBRATE_INVERTER: {
                    FOCUS_LOG_DBG("starting inverter calibration");
                    foc->state_current = foc->state_requested;
                    foc->state_requested = FOCUS_FOC_STATE_NONE;
                    const struct focus_event e = {
                        .type = FOCUS_EVENT_TYPE_SRV_CONTROL_CALIBRATE,
                    };
                    focus_send_event(foc->srv.inverter, &e);
                } break;
                case FOCUS_FOC_STATE_CALIBRATE_POSITION: {

                } break;
                case FOCUS_FOC_STATE_CALIBRATE_MOTOR: {

                } break;
                case FOCUS_FOC_STATE_CALIBRATE_FULL: {

                } break;
                default: {

                } break;
            }
        } break;
        default: {

        } break;
    }
}

void focus_foc_driver(struct focus_srv_control *srv, const struct focus_event *event) {
    struct focus_foc *foc = focus_container_of(srv, struct focus_foc, srv);

    switch(foc->state_current) {
        case FOCUS_FOC_STATE_NONE: {
            FOCUS_LOG_ERR("invalid state");
            foc->state_current = FOCUS_FOC_STATE_IDLE;
        } break;
        case FOCUS_FOC_STATE_CALIBRATE_INVERTER: {
            if(event->type == FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_ENDED) {
                foc->state_current = FOCUS_FOC_STATE_IDLE;
            }
        } break;
        case FOCUS_FOC_STATE_CALIBRATE_POSITION: {
        } break;
        case FOCUS_FOC_STATE_CALIBRATE_MOTOR: {
        } break;
        case FOCUS_FOC_STATE_CALIBRATE_FULL: {
        } break;
        case FOCUS_FOC_STATE_RUNNING: {
            switch(event->type) {
                case FOCUS_EVENT_TYPE_SRV_POSITION_SAMPLE: {

                } break;
                case FOCUS_EVENT_TYPE_SRV_INVERTER_SAMPLE: {

                } break;
                default: {

                } break;
            }
        } break;
        default: {

        } break;
    }
}
