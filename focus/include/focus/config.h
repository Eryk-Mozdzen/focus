#ifndef FOCUS_CONFIG_H_
#define FOCUS_CONFIG_H_

#include "focus_config.h"

#ifndef FOCUS_CONFIG_NUMBER_OF_MOTORS
#define FOCUS_CONFIG_NUMBER_OF_MOTORS 1
#endif

#ifndef FOCUS_CONFIG_MOTOR_POLE_PAIRS
#error "FOCUS_CONFIG_MOTOR_POLE_PAIRS not defined"
#endif

#ifdef FOCUS_CONFIG_ENCODER_ABI
#ifndef FOCUS_CONFIG_ENCODER_CPR
#error "encoder is in use but FOCUS_CONFIG_ENCODER_CPR still not defined"
#endif
#endif

#ifndef FOCUS_CONFIG_SAMPLE_FREQUENCY
#error "FOCUS_CONFIG_SAMPLE_FREQUENCY not defined"
#endif

#ifndef FOCUS_CONFIG_FOC_BANDWIDTH
#define FOCUS_CONFIG_FOC_BANDWIDTH 50.f
#endif

#define FOCUS_CONFIG_SAMPLE_PERIOD (1.f / ((float)FOCUS_CONFIG_SAMPLE_FREQUENCY))

#endif
