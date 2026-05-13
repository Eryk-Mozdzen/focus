#ifndef FOCUS_BIQUAD_H
#define FOCUS_BIQUAD_H

typedef struct {
    float num[3];
    float den[3];
    float x[3];
    float y[3];
} focus_biquad_t;

void focus_biquad_design_lowpass(focus_biquad_t *biquad,
                                 const float frequency_cutoff,
                                 const float frequency_sampling);
void focus_biquad_start(focus_biquad_t *biquad);
float focus_biquad_update(focus_biquad_t *biquad, const float input);

#endif
