#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

#define STATUS_OK 0

#define STATUS_RETURN(expr)                                                                        \
    do {                                                                                           \
        const status_t status = (expr);                                                            \
        if(status != STATUS_OK) {                                                                  \
            return status;                                                                         \
        }                                                                                          \
    } while(0);

typedef uint32_t status_t;

#endif
