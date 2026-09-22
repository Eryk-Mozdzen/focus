#ifndef FOCUS_INVERTER_H
#define FOCUS_INVERTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "focus/common.h"

struct focus_inverter {
    struct focus_srv_inverter srv;

    struct {
        float offset[3];
        float scale[3];
    } params;

    struct {
        volatile uint32_t num;
        volatile uint32_t state;
        volatile float time;
        // volatile float buffer_u[FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES];
        // volatile float buffer_v[FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES];
        // volatile float buffer_w[FOCUS_CONFIG_CURRENT_CALIBRATION_SAMPLES];
    } calibration;
};

void focus_inverter_driver(struct focus_srv_inverter *srv, struct focus_event *event);

#ifdef __cplusplus
}
#endif

#endif
