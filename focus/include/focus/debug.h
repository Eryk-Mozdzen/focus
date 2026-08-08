#ifndef FOCUS_DEBUG_H
#define FOCUS_DEBUG_H

#include <stdint.h>

#include "focus/config.h"

#define FOCUS_DEBUG_BUFFER_APPEND(_voltage_vbus, _current_u, _current_v, _current_w, _current_d,   \
                                  _current_q, _current_q_setpoint, _voltage_d, _voltage_q,         \
                                  _theta_e, _theta_m)                                              \
    do {                                                                                           \
        if(_focus_debug_buffer_index < FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES) {                        \
            _focus_debug_buffer[_focus_debug_buffer_index].voltage_vbus = (_voltage_vbus);         \
            _focus_debug_buffer[_focus_debug_buffer_index].current_uvw[0] = (_current_u);          \
            _focus_debug_buffer[_focus_debug_buffer_index].current_uvw[1] = (_current_v);          \
            _focus_debug_buffer[_focus_debug_buffer_index].current_uvw[2] = (_current_w);          \
            _focus_debug_buffer[_focus_debug_buffer_index].current_dq[0] = (_current_d);           \
            _focus_debug_buffer[_focus_debug_buffer_index].current_dq[1] = (_current_q);           \
            _focus_debug_buffer[_focus_debug_buffer_index].current_q_setpoint =                    \
                (_current_q_setpoint);                                                             \
            _focus_debug_buffer[_focus_debug_buffer_index].voltage_dq[0] = (_voltage_d);           \
            _focus_debug_buffer[_focus_debug_buffer_index].voltage_dq[1] = (_voltage_q);           \
            _focus_debug_buffer[_focus_debug_buffer_index].theta_em[0] = (_theta_e);               \
            _focus_debug_buffer[_focus_debug_buffer_index].theta_em[1] = (_theta_m);               \
            _focus_debug_buffer_index++;                                                           \
        }                                                                                          \
    } while(0)

typedef struct {
    float voltage_vbus;
    float current_uvw[3];
    float current_dq[2];
    float current_q_setpoint;
    float voltage_dq[2];
    float theta_em[2];
    float spare[5];
} focus_debug_t;

extern volatile focus_debug_t _focus_debug_buffer[FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES];
extern volatile uint32_t _focus_debug_buffer_index;

#endif
