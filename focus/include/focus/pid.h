#ifndef FOCUS_PID_H
#define FOCUS_PID_H

#include <stdbool.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float ka;

    volatile float error_prev;
    volatile float antiwindup_prev;
    volatile float integral;
} focus_pid_t;

void focus_pid_set_kp(focus_pid_t *pid, const float kp);
void focus_pid_set_ki(focus_pid_t *pid, const float ki);
void focus_pid_set_kd(focus_pid_t *pid, const float kd);
void focus_pid_set_ka(focus_pid_t *pid, const float ka);
void focus_pid_start(focus_pid_t *pid);
float focus_pid_calculate(focus_pid_t *pid,
                          const float setpoint,
                          const float process_value,
                          const float dt);
void focus_pid_antiwindup(focus_pid_t *pid, const float overflow, const float dt);

#endif
