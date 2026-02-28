#ifndef FOCUS_PORT_H
#define FOCUS_PORT_H

#include <stdint.h>

typedef struct {
    uint32_t encoder_count;
    float current_phase_u;
    float current_phase_v;
    float current_phase_w;
    float voltage_vbus;
} focus_port_sample_t;

typedef struct {
    float duty_cycle_u;
    float duty_cycle_v;
    float duty_cycle_w;
} focus_port_control_t;

void focus_port_event_index(const uint32_t encoder);
void focus_port_event_sample(const focus_port_sample_t *sample);
void focus_port_event_panic();

void focus_port_init(void *user);
void focus_port_start(void *user);
void focus_port_shutdown(void *user);
void focus_port_control(const focus_port_control_t *control, void *user);
float focus_port_timebase(void *user);

#endif
