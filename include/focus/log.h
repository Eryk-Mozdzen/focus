#ifndef FOCUS_LOG_H
#define FOCUS_LOG_H

#include "focus_config.h"

#ifndef FOCUS_LOG_LEVEL
#define FOCUS_LOG_LEVEL FOCUS_LOG_LEVEL_INFO
#endif

#ifndef FOCUS_LOG_MODULE_FILTER
#define FOCUS_LOG_MODULE_FILTER FOCUS_LOG_MODULE_ALL
#endif

#ifdef FOCUS_LOG_ENABLE

#define FOCUS_LOG(level, module, file, line, ...)                                                                      \
    do {                                                                                                               \
        if(((level) >= FOCUS_LOG_LEVEL) && ((module) & FOCUS_LOG_MODULE_FILTER)) {                                     \
            FOCUS_LOG_DRIVER(level, module, file, line, __VA_ARGS__);                                                  \
        }                                                                                                              \
    } while(0)

#define FOCUS_LOG_DBG(...)  FOCUS_LOG(FOCUS_LOG_LEVEL_DEBUG, FOCUS_LOG_MODULE, __FILE__, __LINE__, __VA_ARGS__)
#define FOCUS_LOG_INFO(...) FOCUS_LOG(FOCUS_LOG_LEVEL_INFO, FOCUS_LOG_MODULE, __FILE__, __LINE__, __VA_ARGS__)
#define FOCUS_LOG_WARN(...) FOCUS_LOG(FOCUS_LOG_LEVEL_WARNING, FOCUS_LOG_MODULE, __FILE__, __LINE__, __VA_ARGS__)
#define FOCUS_LOG_ERR(...)  FOCUS_LOG(FOCUS_LOG_LEVEL_ERROR, FOCUS_LOG_MODULE, __FILE__, __LINE__, __VA_ARGS__)

#else

#define FOCUS_LOG_DBG(...)  (void)0
#define FOCUS_LOG_INFO(...) (void)0
#define FOCUS_LOG_WARN(...) (void)0
#define FOCUS_LOG_ERR(...)  (void)0

#endif

enum focus_log_level {
    FOCUS_LOG_LEVEL_DEBUG,
    FOCUS_LOG_LEVEL_INFO,
    FOCUS_LOG_LEVEL_WARNING,
    FOCUS_LOG_LEVEL_ERROR,
};

enum focus_log_module {
    FOCUS_LOG_MODULE_ALL = 0xFFFFFFFFULL,
    FOCUS_LOG_MODULE_SRV_CONTROL = (1ULL << 0),
    FOCUS_LOG_MODULE_SRV_POSITION = (1ULL << 1),
    FOCUS_LOG_MODULE_SRV_INVERTER = (1ULL << 2),
    FOCUS_LOG_MODULE_PORT_POSITION = (1ULL << 3),
    FOCUS_LOG_MODULE_PORT_INVERTER = (1ULL << 4),
};

#endif
