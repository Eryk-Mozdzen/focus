#include "focus/hfi.h"
#include "focus/biquad.h"
#include "focus/math.h"
#include "focus/pid.h"

#ifdef FOCUS_CONFIG_SENSORLESS_HFI

void focus_hfi_init(focus_hfi_t *hfi,
                    const float pll_kp,
                    const float pll_ki,
                    const float injected_amplitude,
                    const float injected_frequency,
                    const float initial_theta_e,
                    const float initial_omega_e) {
    focus_pid_set_kp(&hfi->pid_pll, pll_kp);
    focus_pid_set_ki(&hfi->pid_pll, pll_ki);
    focus_pid_set_kd(&hfi->pid_pll, 0.f);
    focus_pid_set_ka(&hfi->pid_pll, 0.f);
    focus_pid_start(&hfi->pid_pll);
    hfi->pid_pll.integral = initial_omega_e / pll_ki;

    focus_biquad_init_bandstop(&hfi->filter_i_dq[0], injected_frequency,
                               FOCUS_CONFIG_SAMPLING_FREQUENCY);
    focus_biquad_init_bandstop(&hfi->filter_i_dq[1], injected_frequency,
                               FOCUS_CONFIG_SAMPLING_FREQUENCY);

    focus_biquad_start(&hfi->filter_i_dq[0]);
    focus_biquad_start(&hfi->filter_i_dq[1]);

    focus_math_sdft_start(&hfi->sdft, hfi->sdft_buffer, FOCUS_HFI_SDFT_WINDOW);

    hfi->injected_amplitude = injected_amplitude;
    hfi->injected_phase_increment = FOCUS_2PI * injected_frequency * FOCUS_CONFIG_SAMPLING_PERIOD;

    hfi->injected_phase = 0.f;
    hfi->theta_e = focus_math_angle_wrap(initial_theta_e);
    hfi->omega_e = initial_omega_e;
}

void focus_hfi_filter(focus_hfi_t *hfi, float i_dq[2]) {
    i_dq[0] = focus_biquad_update(&hfi->filter_i_dq[0], i_dq[0]);
    i_dq[1] = focus_biquad_update(&hfi->filter_i_dq[1], i_dq[1]);
}

void focus_hfi_inject(focus_hfi_t *hfi, float u_dq[2]) {
    u_dq[0] += (hfi->injected_amplitude * focus_math_sin(hfi->injected_phase));
}

void focus_hfi_update(focus_hfi_t *hfi, const float i_dq[2]) {
    float sdft_amplitude;
    float sdft_phase;
    focus_math_sdft_update(&hfi->sdft, i_dq[1], &sdft_amplitude, &sdft_phase);

    const float phase_correction =
        ((float)(FOCUS_HFI_SDFT_WINDOW - 5) / ((float)FOCUS_HFI_SDFT_WINDOW)) * FOCUS_2PI;

    const float i_qh = focus_math_inverse_dft(sdft_amplitude, sdft_phase + phase_correction);
    const float i_qh_cos = i_qh * focus_math_cos(hfi->injected_phase);
    const float pll_error = sdft_amplitude * focus_math_sign(i_qh_cos);

    const float omega_e =
        focus_pid_calculate(&hfi->pid_pll, 0.f, pll_error, FOCUS_CONFIG_SAMPLING_PERIOD);

    hfi->theta_e += (0.5f * (hfi->omega_e + omega_e) * FOCUS_CONFIG_SAMPLING_PERIOD);
    hfi->theta_e = focus_math_angle_wrap(hfi->theta_e);
    hfi->omega_e = omega_e;

    hfi->injected_phase += hfi->injected_phase_increment;
    if(hfi->injected_phase > FOCUS_PI) {
        hfi->injected_phase -= FOCUS_2PI;
    }
}

#endif
