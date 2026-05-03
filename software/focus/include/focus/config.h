#ifndef FOCUS_CONFIG_H_
#define FOCUS_CONFIG_H_

#include "focus_config.h"

#ifndef FOCUS_CONFIG_MOTOR_POLE_PAIRS
#error "FOCUS_CONFIG_MOTOR_POLE_PAIRS not defined"
#endif

#ifndef FOCUS_CONFIG_ENCODER_CPR
#error "FOCUS_CONFIG_ENCODER_CPR not defined"
#endif

#ifndef FOCUS_CONFIG_SAMPLE_PERIOD
#error "FOCUS_CONFIG_SAMPLE_PERIOD not defined"
#endif

#ifndef FOCUS_CONFIG_PANIC_DURATION
#define FOCUS_CONFIG_PANIC_DURATION 1.f
#endif

#ifndef FOCUS_MOTOR_BANDWIDTH
#define FOCUS_MOTOR_BANDWIDTH 1000.f
#endif

#ifndef FOCUS_NUMBER_OF_MOTORS
#define FOCUS_NUMBER_OF_MOTORS 1
#endif

#endif
