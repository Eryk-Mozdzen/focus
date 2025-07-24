#ifndef DRV_AS5600_H
#define DRV_AS5600_H

#include "focus/encoder.h"

typedef struct {
    encoder_if_t interface;
} drv_as5600_t;

status_t drv_as5600_init(void *driver);
status_t drv_as5600_deinit(void *driver);
status_t drv_as5600_sample_start(void *driver);
status_t drv_as5600_sample_get(void *driver, float *sample);

#endif
