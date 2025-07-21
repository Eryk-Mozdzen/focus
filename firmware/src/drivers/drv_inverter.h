#ifndef DRV_INVERTER_H
#define DRV_INVERTER_H

#include "system/if_inverter.h"

typedef struct {
    if_inverter_t interface;
} drv_inverter_t;

status_t drv_inverter_init(void *driver);
status_t drv_inverter_deinit(void *driver);
status_t drv_inverter_set_duty_cycle(void *driver, const float u, const float v, const float w);
status_t drv_inverter_get_current(void *driver, float *u, float *v, float *w);

#endif
