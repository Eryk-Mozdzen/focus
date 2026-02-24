#ifndef FOCUS_API_H
#define FOCUS_API_H

typedef struct {
    float supply;
    float current_uvw[3];
    float current_dq[2];
    float current_setpoint;
    float position;
    float velocity;
} focus_context_t;

typedef enum {
    FOCUS_STATE_PANIC,
    FOCUS_STATE_RUNNING,
} focus_state_t;

void focus_init(focus_context_t *context, void *user);
void focus_task();

#endif
