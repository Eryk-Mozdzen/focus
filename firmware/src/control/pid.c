#include "control/pid.h"

#define K_ANTIWINDUP 1.f

void pid_set_kp(pid_t *pid, const float kp) {
    pid->kp = kp;
}

void pid_set_ki(pid_t *pid, const float ki) {
    pid->ki = ki;
}

void pid_set_kd(pid_t *pid, const float kd) {
    pid->kd = kd;
}

void pid_antiwindup_enable(pid_t *pid, const bool enable) {
    pid->antiwindup_enable = enable;
}

void pid_antiwindup_set_min(pid_t *pid, const float min) {
    pid->output_min = min;
}

void pid_antiwindup_set_max(pid_t *pid, const float max) {
    pid->output_max = max;
}

void pid_start(pid_t *pid) {
    pid->error_prev = 0;
    pid->error_prev_antiwindup = 0;
    pid->error_integral = 0;
    pid->output_unconstrained = 0;
    pid->output_constrained = 0;
}

float pid_calculate(pid_t *pid,
                    const float setpoint,
                    const float process_value,
                    const float delta_time) {
    const float error = setpoint - process_value;

    if(pid->antiwindup_enable) {
        const float error_antiwindup =
            error + (pid->output_constrained - pid->output_unconstrained) * K_ANTIWINDUP;

        pid->error_integral += 0.5f * (error_antiwindup + pid->error_prev) * delta_time;
        pid->error_prev_antiwindup = error_antiwindup;
    } else {
        pid->error_integral += 0.5f * (error + pid->error_prev) * delta_time;
    }

    const float derivative = (error - pid->error_prev) / delta_time;

    pid->error_prev = error;

    pid->output_unconstrained =
        (pid->kp * error) + (pid->ki * pid->error_integral) + (pid->kd * derivative);

    if(pid->antiwindup_enable) {
        if(pid->output_unconstrained > pid->output_max) {
            pid->output_constrained = pid->output_max;
        } else if(pid->output_unconstrained < pid->output_min) {
            pid->output_constrained = pid->output_min;
        } else {
            pid->output_constrained = pid->output_unconstrained;
        }

        return pid->output_constrained;
    }

    return pid->output_unconstrained;
}
