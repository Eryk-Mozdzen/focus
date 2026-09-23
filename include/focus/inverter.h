#ifndef FOCUS_INVERTER_H
#define FOCUS_INVERTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "focus/common.h"

enum focus_inverter_state {
    FOCUS_INVERTER_STATE_WAITING_FOR_CALIBRATION,
    FOCUS_INVERTER_STATE_CALIBRATION_OFFSET_UVW,
    FOCUS_INVERTER_STATE_CALIBRATION_SCALE_U,
    FOCUS_INVERTER_STATE_CALIBRATION_SCALE_V,
    FOCUS_INVERTER_STATE_CALIBRATION_SCALE_W,
    FOCUS_INVERTER_STATE_CALIBRATED,
};

struct focus_inverter {
    struct focus_srv_inverter srv;

    struct {
        struct {
            volatile float offset[3];
            volatile float scale[3];
        } params;
        uint32_t calibration_offset_samples;
        uint32_t calibration_scale_samples;
        float calibration_scale_voltage;
    } config;

    volatile enum focus_inverter_state state;
    volatile float accumulator_uvw[3];
    volatile uint32_t accumulator_counter;
};

void focus_inverter_driver(struct focus_srv_inverter *srv, const struct focus_event *event);

#ifdef __cplusplus
}
#endif

#endif
