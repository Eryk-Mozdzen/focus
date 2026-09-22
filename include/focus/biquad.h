#ifndef FOCUS_BIQUAD_H
#define FOCUS_BIQUAD_H

#ifdef __cplusplus
extern "C" {
#endif

struct focus_biquad {
    float num[3];
    float den[3];
    float x[3];
    float y[3];
};

void focus_biquad_design_lowpass(struct focus_biquad *biquad,
                                 const float frequency_cutoff,
                                 const float frequency_sampling);
void focus_biquad_start(struct focus_biquad *biquad);
float focus_biquad_update(struct focus_biquad *biquad, const float input);

#ifdef __cplusplus
}
#endif

#endif
