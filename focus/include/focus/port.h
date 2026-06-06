#ifndef FOCUS_PORT_H
#define FOCUS_PORT_H

#include <stdint.h>

#include "focus/config.h"

typedef struct {
#ifdef FOCUS_CONFIG_ENCODER_ABI
    uint32_t encoder_count;
#endif
    float current_u;
    float current_v;
    float current_w;
    float voltage_vbus;
} focus_port_sample_t;

typedef struct {
    float duty_cycle_u;
    float duty_cycle_v;
    float duty_cycle_w;
} focus_port_control_t;

#ifdef FOCUS_CONFIG_ENCODER_ABI
void focus_port_event_index(const uint32_t motor, const uint32_t encoder_count);
#endif
void focus_port_event_sample(const uint32_t motor, const focus_port_sample_t *sample);
void focus_port_event_panic(const uint32_t motor);

void focus_port_init(void *user);
void focus_port_start(const uint32_t motor, void *user);
void focus_port_shutdown(const uint32_t motor, void *user);
void focus_port_control(const uint32_t motor, const focus_port_control_t *control, void *user);
float focus_port_timebase(void *user);

#endif
