#ifndef FOCUS_HFI_H
#define FOCUS_HFI_H

#include "focus/biquad.h"
#include "focus/config.h"
#include "focus/math.h"
#include "focus/pid.h"

#ifdef FOCUS_CONFIG_SENSORLESS_HFI

#define FOCUS_HFI_SDFT_WINDOW                                                                      \
    (FOCUS_CONFIG_SAMPLING_FREQUENCY / FOCUS_CONFIG_SENSORLESS_HFI_INJECTED_FREQUENCY)

#define FOCUS_HFI_GET_ELECTRICAL_POSITION(hfi) ((hfi)->theta_e)
#define FOCUS_HFI_GET_ELECTRICAL_VELOCITY(hfi) ((hfi)->omega_e)

typedef struct {
    focus_biquad_t filter_i_dq[2];
    focus_pid_t pid_pll;
    focus_math_sdft_t sdft;
    float sdft_buffer[FOCUS_HFI_SDFT_WINDOW];
    float injected_amplitude;
    float injected_phase_increment;
    volatile float injected_phase;
    volatile float theta_e;
    volatile float omega_e;
} focus_hfi_t;

void focus_hfi_init(focus_hfi_t *hfi,
                    const float pll_kp,
                    const float pll_ki,
                    const float injected_amplitude,
                    const float injected_frequency,
                    const float initial_theta_e,
                    const float initial_omega_e);
void focus_hfi_filter(focus_hfi_t *hfi, float i_dq[2]);
void focus_hfi_inject(focus_hfi_t *hfi, float u_dq[2]);
void focus_hfi_update(focus_hfi_t *hfi, const float i_dq[2]);

#endif

#endif
