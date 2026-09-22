#ifndef FOCUS_CONFIG_H
#define FOCUS_CONFIG_H

#include "log_sink.h"

#define FOCUS_LOG_ENABLE
#define FOCUS_LOG_LEVEL         FOCUS_LOG_LEVEL_INFO
#define FOCUS_LOG_MODULE_FILTER FOCUS_LOG_MODULE_ALL
#define FOCUS_LOG_SINK(level, module, file, line, ...)                                             \
    log_sink(level, module, file, line, __VA_ARGS__)

#endif
