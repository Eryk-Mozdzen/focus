#ifndef FOCUS_SMO_H
#define FOCUS_SMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "focus/biquad.h"
#include "focus/common.h"

struct focus_smo {
    struct focus_srv_position srv;

    struct {
        struct {
            float g;
            float eta;
        } observer;
        struct {
            float align_time;
            float align_voltage;
            float duration;
            float voltage;
            float time_constant;
            float velocity;
        } ramp;
        float sampling_frequency;
        float filter_bandwidth;
    } params;

    volatile float a;
    volatile float b;

    volatile float i_ab_estimate[2];
    volatile float e_ab_estimate[2];
    volatile float i_ab_residual_prev[2];

    volatile float theta_e;
    volatile float omega_e;
    struct focus_biquad omega_e_filter;
};

void focus_smo_driver(struct focus_srv_position *srv, struct focus_event *event);

#ifdef __cplusplus
}
#endif

#endif
