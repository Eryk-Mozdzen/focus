#include <stdint.h>

#include "focus/config.h"
#include "focus/debug.h"

volatile focus_debug_t _focus_debug_buffer[FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES];
volatile uint32_t _focus_debug_buffer_index;
