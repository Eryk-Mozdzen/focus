#ifndef FOCUS_API_H
#define FOCUS_API_H

typedef struct {
    float supply;
    float current_uvw[3];
    float current_dq[2];
    float current_setpoint;
    float position;
    float position_open_loop;
} focus_context_t;

typedef enum {
    FOCUS_STATE_IDLE,
    FOCUS_STATE_RUNNING,
    FOCUS_STATE_PANIC,
} focus_state_t;

void focus_init(focus_context_t *context, void *user);
void focus_task();

void focus_request_state(const focus_state_t requested_state);

#endif
