#ifndef FOCUS_CONFIG_H_
#define FOCUS_CONFIG_H_

#include "focus_config.h"

#define FOCUS_CONFIG_SAMPLING_PERIOD (1.f / ((float)FOCUS_CONFIG_SAMPLING_FREQUENCY))

#if (defined(FOCUS_CONFIG_SENSORLESS_ENABLE) && defined(FOCUS_CONFIG_ENCODER_ENABLE))
#error "you can use SENSORLESS or ENCODER in one time"
#endif

#ifdef FOCUS_CONFIG_ENCODER_ENABLE
#if ((!defined(FOCUS_CONFIG_ENCODER_AB)) && (!defined(FOCUS_CONFIG_ENCODER_ABI)) &&                \
     (!defined(FOCUS_CONFIG_ENCODER_ABSOLUTE)))
#error "you need to specify AB, ABI or ABSOLUTE type of encoder"
#endif
#endif

#endif
