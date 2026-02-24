#ifndef FOCUS_PORT_H
#define FOCUS_PORT_H

typedef struct {
    float position_mechanical;
    float current_u;
    float current_v;
    float current_w;
    float supply_voltage;
} focus_port_sample_t;

typedef struct {
    float duty_cycle_u;
    float duty_cycle_v;
    float duty_cycle_w;
} focus_port_control_t;

void focus_port_event_sample(const focus_port_sample_t *sample);
void focus_port_event_panic();

void focus_port_init(void *user);
void focus_port_start(void *user);
void focus_port_shutdown(void *user);
void focus_port_control(const focus_port_control_t *control, void *user);
float focus_port_timebase(void *user);

#endif
