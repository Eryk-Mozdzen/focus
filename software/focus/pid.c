#include "focus/pid.h"

void focus_pid_set_kp(focus_pid_t *pid, const float kp) {
    pid->kp = kp;
}

void focus_pid_set_ki(focus_pid_t *pid, const float ki) {
    pid->ki = ki;
}

void focus_pid_set_kd(focus_pid_t *pid, const float kd) {
    pid->kd = kd;
}

void focus_pid_antiwindup_enable(focus_pid_t *pid, const bool enable) {
    pid->antiwindup_enable = enable;
}

void focus_pid_antiwindup_set_min(focus_pid_t *pid, const float min) {
    pid->output_min = min;
}

void focus_pid_antiwindup_set_max(focus_pid_t *pid, const float max) {
    pid->output_max = max;
}

void focus_pid_start(focus_pid_t *pid) {
    pid->error_prev = 0;
    pid->error_prev_antiwindup = 0;
    pid->error_integral = 0;
    pid->output_unconstrained = 0;
    pid->output_constrained = 0;
}

float focus_pid_calculate(focus_pid_t *pid,
                          const float setpoint,
                          const float process_value,
                          const float dt) {
    const float error = setpoint - process_value;

    if(pid->antiwindup_enable) {
        const float error_antiwindup =
            error + (pid->output_constrained - pid->output_unconstrained) * K_ANTIWINDUP;

        pid->error_integral += 0.5f * (error_antiwindup + pid->error_prev) * dt;
        pid->error_prev_antiwindup = error_antiwindup;
    } else {
        pid->error_integral += 0.5f * (error + pid->error_prev) * dt;
    }

    const float derivative = (error - pid->error_prev) / dt;

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
