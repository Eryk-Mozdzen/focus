#include <stdbool.h>
#include <stdint.h>

#include "focus/biquad.h"
#include "focus/common.h"
#include "focus/debug.h"
#include "focus/foc.h"
#include "focus/math.h"
#include "focus/pid.h"

void focus_foc_init(struct focus_foc *foc) {
    foc->srv.inverter->control = &foc->srv;
    foc->srv.position->control = &foc->srv;

    foc->state_current = FOCUS_FOC_STATE_IDLE;
    foc->state_requested = FOCUS_FOC_STATE_NONE;
}

void focus_foc_req_state(struct focus_foc *foc, enum focus_foc_state state) {
    foc->state_requested = state;
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
    switch(foc->state_current) { case: }
}

void focus_foc_driver(struct focus_srv_control *srv, struct focus_event *event) {
    struct focus_foc *foc = focus_container_of(srv, struct focus_foc, srv);

    switch(event->type) {
        case FOCUS_EVENT_TYPE_FOC_SAMPLE_INVERTER: {

        } break;
        case FOCUS_EVENT_TYPE_FOC_SAMPLE_POSITION: {

        } break;
    }
}
