#ifndef FOCUS_MATH_H
#define FOCUS_MATH_H

#include <stdint.h>

#define FOCUS_PI    3.14159265359f
#define FOCUS_2PI   6.283185307f
#define FOCUS_SQRT3 1.732050808f

float focus_math_clamp(const float x, const float min, const float max);
void focus_math_clamp_vector(const float in[2], const float max_len, float out[2]);
float focus_math_wrap(const float in);
int32_t focus_math_lerp(const int32_t x1,
                        const int32_t y1,
                        const int32_t x2,
                        const int32_t y2,
                        const int32_t xi);
void focus_math_clark_transform(const float i_uvw[3], float i_ab[2]);
void focus_math_park_transform(const float i_ab[2], const float theta, float i_dq[2]);
void focus_math_inverse_park_transform(const float u_dq[2], const float theta, float u_ab[2]);
void focus_math_inverse_clark_transform(const float u_ab[2], float u_uvw[3]);
void focus_math_svpwm(const float u_ab[2], float u_supply, float duty_cycle_uvw[3]);
void focus_math_single_frequency_dft(const float *signal,
                                     const uint32_t signal_length,
                                     const float signal_sample_period,
                                     const float target_frequency,
                                     float *amplitude,
                                     float *phase,
                                     float *bias);

#endif
