#ifndef FOCUS_SMO_H
#define FOCUS_SMO_H

#include "focus/biquad.h"

#define FOCUS_SMO_GET_ELECTRICAL_POSITION(smo) ((smo)->theta_e)
#define FOCUS_SMO_GET_ELECTRICAL_VELOCITY(smo) ((smo)->omega_e)

typedef struct {
    volatile float a;
    volatile float b;
    volatile float eta;

    volatile float i_ab_estimate[2];
    volatile float e_ab_estimate[2];
    volatile float i_ab_residual_prev[2];

    volatile float theta_e;
    volatile float omega_e;
    focus_biquad_t omega_e_filter;
} focus_smo_t;

void focus_smo_init(focus_smo_t *smo, const float rs, const float ld, const float lq);
void focus_smo_update(focus_smo_t *smo, const float u_ab[2], const float i_ab[2]);

#endif
