#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

#define STATUS_RETURN(expr)                                                                        \
    do {                                                                                           \
        const status_t status = (expr);                                                            \
        if(status != STATUS_OK) {                                                                  \
            return status;                                                                         \
        }                                                                                          \
    } while(0);

typedef enum {
    STATUS_OK,

    STATUS_SYSTEM_INVALID_IMPL,
    STATUS_SYSTEM_INVALID_ARG,

    STATUS_MOTOR_OVERCURRENT,
    STATUS_MOTOR_OVERVOLTAGE,
    STATUS_MOTOR_UNDERVOLTAGE,
} status_t;

#endif
