#ifndef FOCUS_PID_H
#define FOCUS_PID_H

#ifdef __cplusplus
extern "C" {
#endif

struct focus_pid {
    float kp;
    float ki;
    float kd;
    float ka;

    volatile float error_prev;
    volatile float antiwindup_prev;
    volatile float integral;
};

void focus_pid_set_kp(struct focus_pid *pid, const float kp);
void focus_pid_set_ki(struct focus_pid *pid, const float ki);
void focus_pid_set_kd(struct focus_pid *pid, const float kd);
void focus_pid_set_ka(struct focus_pid *pid, const float ka);
void focus_pid_start(struct focus_pid *pid);
float focus_pid_calculate(struct focus_pid *pid,
                          const float setpoint,
                          const float process_value,
                          const float dt);
void focus_pid_antiwindup(struct focus_pid *pid, const float overflow, const float dt);

#ifdef __cplusplus
}
#endif

#endif
