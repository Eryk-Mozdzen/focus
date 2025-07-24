#include "drv_inverter.h"

status_t drv_inverter_init(void *driver) {
    (void)driver;
    return STATUS_OK;
}

status_t drv_inverter_deinit(void *driver) {
    (void)driver;
    return STATUS_OK;
}

status_t drv_inverter_set_duty_cycle(void *driver, const float u, const float v, const float w) {
    (void)driver;
    (void)u;
    (void)v;
    (void)w;
    return STATUS_OK;
}

status_t drv_inverter_get_current(void *driver, float *u, float *v, float *w) {
    (void)driver;
    (void)u;
    (void)v;
    (void)w;
    return STATUS_OK;
}

status_t drv_inverter_get_supply(void *driver, float *supply) {
    (void)driver;
    (void)supply;
    return STATUS_OK;
}
