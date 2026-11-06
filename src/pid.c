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

void focus_pid_set_ka(focus_pid_t *pid, const float ka) {
    pid->ka = ka;
}

void focus_pid_start(focus_pid_t *pid) {
    pid->error_prev = 0;
    pid->antiwindup_prev = 0;
    pid->integral = 0;
}

float focus_pid_calculate(focus_pid_t *pid,
                          const float setpoint,
                          const float process_value,
                          const float dt) {
    const float error = setpoint - process_value;

    const float derivative = (error - pid->error_prev) / dt;

    pid->integral += 0.5f * (pid->error_prev + error) * dt;
    pid->error_prev = error;

    const float output_unconstrained =
        (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);

    return output_unconstrained;
}

void focus_pid_antiwindup(focus_pid_t *pid, const float overflow, const float dt) {
    pid->integral += (pid->ka * (0.5f * (pid->antiwindup_prev + overflow)) * dt);
    pid->antiwindup_prev = overflow;
}
