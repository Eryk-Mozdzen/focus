#ifndef FOCUS_DEBUG_H
#define FOCUS_DEBUG_H

#include <stdint.h>

#include "focus/config.h"

#define FOCUS_DEBUG_BUFFER_APPEND(ch1, ch2, ch3)                                                   \
    do {                                                                                           \
        if(_focus_debug_buffer_index < FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES) {                        \
            _focus_debug_buffer[_focus_debug_buffer_index][0] = (ch1);                             \
            _focus_debug_buffer[_focus_debug_buffer_index][1] = (ch2);                             \
            _focus_debug_buffer[_focus_debug_buffer_index][2] = (ch3);                             \
            _focus_debug_buffer_index++;                                                           \
        }                                                                                          \
    } while(0)

extern volatile float _focus_debug_supply;
extern volatile float _focus_debug_position_ol;
extern volatile float _focus_debug_svpwm[3];
extern volatile float _focus_debug_uvw[3];
extern volatile float _focus_debug_buffer[FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES][3];
extern volatile uint32_t _focus_debug_buffer_index;

#endif
