#ifndef DRV_ENCODER_AS5600_H
#define DRV_ENCODER_AS5600_H

#include "system/if_encoder.h"

typedef struct {
    if_encoder_t interface;
} drv_encoder_as5600_t;

status_t drv_encoder_as5600_init(void *driver);
status_t drv_encoder_as5600_deinit(void *driver);
status_t drv_encoder_as5600_sample_start(void *driver);
status_t drv_encoder_as5600_sample_get(void *driver, float *sample);

#endif
