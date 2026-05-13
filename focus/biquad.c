#include <math.h>

#include "focus/biquad.h"
#include "focus/math.h"

void focus_biquad_design_lowpass(focus_biquad_t *biquad,
                                 const float frequency_cutoff,
                                 const float frequency_sampling) {
    const float w0 = FOCUS_2PI * frequency_cutoff / frequency_sampling;
    const float q = 0.70710678f;
    const float alpha = sinf(w0) / (2.f * q);
    const float cos_w0 = cosf(w0);

    biquad->num[0] = 0.5f * (1.f - cos_w0);
    biquad->num[1] = 1.f - cos_w0;
    biquad->num[2] = 0.5f * (1.f - cos_w0);

    biquad->den[0] = 1.f + alpha;
    biquad->den[1] = -2.f * cos_w0;
    biquad->den[2] = 1.f - alpha;
}

void focus_biquad_start(focus_biquad_t *biquad) {
    biquad->x[0] = 0.f;
    biquad->x[1] = 0.f;
    biquad->x[2] = 0.f;

    biquad->y[0] = 0.f;
    biquad->y[1] = 0.f;
    biquad->y[2] = 0.f;
}

float focus_biquad_update(focus_biquad_t *biquad, const float input) {
    biquad->x[2] = biquad->x[1];
    biquad->x[1] = biquad->x[0];
    biquad->x[0] = input;

    biquad->y[2] = biquad->y[1];
    biquad->y[1] = biquad->y[0];
    biquad->y[0] = ((biquad->num[0] * biquad->x[0]) + (biquad->num[1] * biquad->x[1]) +
                    (biquad->num[2] * biquad->x[2]) - (biquad->den[1] * biquad->y[1]) -
                    (biquad->den[2] * biquad->y[2])) /
                   biquad->den[0];

    return biquad->y[0];
}
