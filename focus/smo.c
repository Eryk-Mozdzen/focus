#include <math.h>

#include "focus/biquad.h"
#include "focus/config.h"
#include "focus/math.h"
#include "focus/smo.h"

#ifdef FOCUS_CONFIG_SENSORLESS

void focus_smo_init(focus_smo_t *smo, const float rs, const float ld, const float lq) {
    const float R = rs;
    const float L = 0.5f * (ld + lq);

    smo->a = expf(-(R * FOCUS_CONFIG_SAMPLE_PERIOD) / L);
    smo->b = (1.f / R) * (1.f - expf(-(R * FOCUS_CONFIG_SAMPLE_PERIOD) / L));
    smo->eta = 2.f * ((smo->b * FOCUS_CONFIG_SENSORLESS_SMO_M) / FOCUS_CONFIG_SENSORLESS_SMO_G);

    smo->i_ab_estimate[0] = 0.f;
    smo->i_ab_estimate[1] = 0.f;

    smo->e_ab_estimate[0] = 0.f;
    smo->e_ab_estimate[1] = 0.f;

    smo->i_ab_residual_prev[0] = 0.f;
    smo->i_ab_residual_prev[1] = 0.f;

    smo->theta_e = 0.f;
    smo->omega_e = 0.f;

    focus_biquad_design_lowpass(&smo->omega_e_filter, FOCUS_CONFIG_SENSORLESS_SMO_VELOCITY_CUTOFF,
                                FOCUS_CONFIG_SAMPLE_PERIOD);
    focus_biquad_start(&smo->omega_e_filter);
}

void focus_smo_update(focus_smo_t *smo, const float u_ab[2], const float i_ab[2]) {
    const float dir_prev = atan2f(smo->e_ab_estimate[1], smo->e_ab_estimate[0]);

    const float i_ab_residual[2] = {
        smo->i_ab_estimate[0] - i_ab[0],
        smo->i_ab_estimate[1] - i_ab[1],
    };

    smo->i_ab_estimate[0] = (smo->a * smo->i_ab_estimate[0]) +
                            (smo->b * (u_ab[0] - smo->e_ab_estimate[0])) -
                            (smo->eta * focus_math_sign(i_ab_residual[0]));
    smo->i_ab_estimate[1] = (smo->a * smo->i_ab_estimate[1]) +
                            (smo->b * (u_ab[1] - smo->e_ab_estimate[1])) -
                            (smo->eta * focus_math_sign(i_ab_residual[1]));

    smo->e_ab_estimate[0] =
        smo->e_ab_estimate[0] + ((FOCUS_CONFIG_SENSORLESS_SMO_G / smo->b) *
                                 (i_ab_residual[0] - (smo->a * smo->i_ab_residual_prev[0]) +
                                  (smo->eta * focus_math_sign(smo->i_ab_residual_prev[0]))));
    smo->e_ab_estimate[1] =
        smo->e_ab_estimate[1] + ((FOCUS_CONFIG_SENSORLESS_SMO_G / smo->b) *
                                 (i_ab_residual[1] - (smo->a * smo->i_ab_residual_prev[1]) +
                                  (smo->eta * focus_math_sign(smo->i_ab_residual_prev[1]))));

    smo->i_ab_residual_prev[0] = i_ab_residual[0];
    smo->i_ab_residual_prev[1] = i_ab_residual[1];

    const float dir_curr = atan2f(smo->e_ab_estimate[1], smo->e_ab_estimate[0]);
    const float omega_e = focus_math_angle_sub(dir_curr, dir_prev) / FOCUS_CONFIG_SAMPLE_PERIOD;

    smo->omega_e = focus_biquad_update(&smo->omega_e_filter, omega_e);
    smo->theta_e = focus_math_angle_sub(dir_curr, focus_math_sign(smo->omega_e) * FOCUS_HALF_PI);
}

#endif
