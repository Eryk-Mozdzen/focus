#ifndef FOCUS_PID_H
#define FOCUS_PID_H

#include <stdbool.h>

typedef struct {
    float kp;
    float ki;
    float kd;

    float error_prev;
    float error_prev_antiwindup;
    float error_integral;

    bool antiwindup_enable;
    float output_min;
    float output_max;
    float output_unconstrained;
    float output_constrained;
} pid_t;

void pid_set_kp(pid_t *pid, const float kp);
void pid_set_ki(pid_t *pid, const float ki);
void pid_set_kd(pid_t *pid, const float kd);
void pid_antiwindup_enable(pid_t *pid, const bool enable);
void pid_antiwindup_set_min(pid_t *pid, const float min);
void pid_antiwindup_set_max(pid_t *pid, const float max);

void pid_start(pid_t *pid);
float pid_calculate(pid_t *pid,
                    const float setpoint,
                    const float process_value,
                    const float delta_time);

#endif
