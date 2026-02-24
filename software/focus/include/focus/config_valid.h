#ifndef FOCUS_CONFIG_VALID_H
#define FOCUS_CONFIG_VALID_H

#include "focus_config.h"

#ifndef FOCUS_CONFIG_MOTOR_POLE_PAIRS
#error "FOCUS need to know how many pole pairs your motor has"
#endif

#ifndef FOCUS_CONFIG_PANIC_DURATION
#define FOCUS_CONFIG_PANIC_DURATION 1.f
#endif

#endif
