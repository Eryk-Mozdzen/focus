#include "focus/smo.h"
#include "focus/biquad.h"
#include "focus/common.h"
#include "focus/math.h"

void focus_smo_driver(struct focus_srv_position *srv, struct focus_event *event) {
    struct focus_smo *smo = focus_container_of(srv, struct focus_smo, srv);

    switch(event->type) {
        case FOCUS_EVENT_TYPE_CONTROL_START: {
            const float ls = 0.5f * (event->arg.control_start.ld + event->arg.control_start.lq);

            smo->a =
                focus_math_exp(-(event->arg.control_start.rs / ls) * FOCUS_CONFIG_SAMPLING_PERIOD);
            smo->b = (1.f - smo->a) / event->arg.control_start.rs;

            smo->i_ab_estimate[0] = 0.f;
            smo->i_ab_estimate[1] = 0.f;

            smo->e_ab_estimate[0] = 0.f;
            smo->e_ab_estimate[1] = 0.f;

            smo->i_ab_residual_prev[0] = 0.f;
            smo->i_ab_residual_prev[1] = 0.f;

            smo->theta_e = 0.f;
            smo->omega_e = 0.f;

            focus_biquad_design_lowpass(&smo->omega_e_filter, smo->params.bandwidth,
                                        FOCUS_CONFIG_SAMPLING_FREQUENCY);
            focus_biquad_start(&smo->omega_e_filter);
        } break;
        case FOCUS_EVENT_TYPE_CONTROL_LOOP: {
            const float dir_prev = focus_math_atan2(smo->e_ab_estimate[1], smo->e_ab_estimate[0]);

            const float *i_ab = event->arg.control_loop.i_ab;
            const float *u_ab = event->arg.control_loop.u_ab;

            const float i_ab_residual[2] = {
                smo->i_ab_estimate[0] - i_ab[0],
                smo->i_ab_estimate[1] - i_ab[1],
            };

            smo->i_ab_estimate[0] = (smo->a * smo->i_ab_estimate[0]) +
                                    (smo->b * (u_ab[0] - smo->e_ab_estimate[0])) -
                                    (smo->params.observer.eta * focus_math_sign(i_ab_residual[0]));
            smo->i_ab_estimate[1] = (smo->a * smo->i_ab_estimate[1]) +
                                    (smo->b * (u_ab[1] - smo->e_ab_estimate[1])) -
                                    (smo->params.observer.eta * focus_math_sign(i_ab_residual[1]));

            smo->e_ab_estimate[0] =
                smo->e_ab_estimate[0] +
                ((smo->params.observer.g / smo->b) *
                 (i_ab_residual[0] - (smo->a * smo->i_ab_residual_prev[0]) +
                  (smo->params.observer.eta * focus_math_sign(smo->i_ab_residual_prev[0]))));
            smo->e_ab_estimate[1] =
                smo->e_ab_estimate[1] +
                ((smo->params.observer.g / smo->b) *
                 (i_ab_residual[1] - (smo->a * smo->i_ab_residual_prev[1]) +
                  (smo->params.observer.eta * focus_math_sign(smo->i_ab_residual_prev[1]))));

            smo->i_ab_residual_prev[0] = i_ab_residual[0];
            smo->i_ab_residual_prev[1] = i_ab_residual[1];

            const float dir_curr = focus_math_atan2(smo->e_ab_estimate[1], smo->e_ab_estimate[0]);
            const float omega_e =
                focus_math_angle_sub(dir_curr, dir_prev) / FOCUS_CONFIG_SAMPLING_PERIOD;

            smo->omega_e = focus_biquad_update(&smo->omega_e_filter, omega_e);
            smo->theta_e =
                focus_math_angle_sub(dir_curr, focus_math_sign(smo->omega_e) * FOCUS_HALF_PI);
        } break;
    }
}
