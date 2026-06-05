#include <stdint.h>

#include "focus/config.h"

volatile float _focus_debug_supply;
volatile float _focus_debug_position_ol;
volatile float _focus_debug_svpwm[3];
volatile float _focus_debug_uvw[3];
volatile float _focus_debug_buffer[FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES][3];
volatile uint32_t _focus_debug_buffer_index;
