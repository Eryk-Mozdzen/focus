#ifndef FOCUS_CONFIG_H
#define FOCUS_CONFIG_H

#include "hw_port.h"

#define FOCUS_LOG_ENABLE
#define FOCUS_LOG_LEVEL         FOCUS_LOG_LEVEL_DEBUG
#define FOCUS_LOG_MODULE_FILTER FOCUS_LOG_MODULE_ALL
#define FOCUS_LOG_SINK(level, module, file, line, ...)                                             \
    hw_port_log(level, module, file, line, __VA_ARGS__)

#endif
