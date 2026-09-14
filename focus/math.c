#include <stddef.h>
#include <stdint.h>

#include "focus/math.h"

// FOCUS_MATH_LOG2E = log2(e) = 1/ln(2)
// FOCUS_MATH_LN2_HI + FOCUS_MATH_LN2_LO = ln(2)
#define FOCUS_MATH_LOG2E     1.44269504088896341f
#define FOCUS_MATH_LN2_HI    0.693359375f
#define FOCUS_MATH_LN2_LO    -2.12194440e-4f
#define FOCUS_MATH_EXP_CLAMP 87.f

typedef union {
    float f;
    int32_t i;
} focus_math_ieee754_t;

extern const int focus_math_lookup_sin_num;
extern const float focus_math_lookup_sin[];

static float focus_math_sin_eval(float x) {
    float sign = 1.f;
    // x is now in [-pi; pi)
    if(x < 0.f) {
        x = -x;
        sign = -1.f;
    }
    // x is now in [0; pi]
    if(x > FOCUS_HALF_PI) {
        x = FOCUS_PI - x;
    }
    // x is now in [0; pi/2]
    const float scaled = x * (((float)focus_math_lookup_sin_num) / FOCUS_HALF_PI);
    int32_t idx = (int32_t)scaled;
    if(idx < 0) {
        idx = 0;
    } else if(idx >= focus_math_lookup_sin_num) {
        idx = focus_math_lookup_sin_num - 1;
    }
    const float frac = scaled - ((float)idx);
    const float a = focus_math_lookup_sin[idx];
    const float b = focus_math_lookup_sin[idx + 1];
    return sign * (a + (frac * (b - a)));
}

float focus_math_abs(float x) {
    return ((x >= 0.f) ? x : -x);
}

float focus_math_sqrt(float x) {
    if(x <= 0.f) {
        return 0.f;
    }
    const float half_x = x * 0.5f;
    focus_math_ieee754_t u;
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1);
    float y = u.f;
    y = y * (1.5f - (half_x * y * y)); // Newton-Raphson iteration 1
    y = y * (1.5f - (half_x * y * y)); // Newton-Raphson iteration 2
    return x * y;
}

float focus_math_exp(float x) {
    if(x > FOCUS_MATH_EXP_CLAMP) {
        x = FOCUS_MATH_EXP_CLAMP;
    }
    if(x < -FOCUS_MATH_EXP_CLAMP) {
        x = -FOCUS_MATH_EXP_CLAMP;
    }
    float fk = (x * FOCUS_MATH_LOG2E) + ((x >= 0.0f) ? 0.5f : -0.5f);
    int32_t k = (int32_t)fk;
    fk = (float)k;
    float r = x - (fk * FOCUS_MATH_LN2_HI);
    r = r - (fk * FOCUS_MATH_LN2_LO);
    const float p0 = 1.9875691500e-4f;
    const float p1 = 1.3981999507e-3f;
    const float p2 = 8.3334519073e-3f;
    const float p3 = 4.1665795894e-2f;
    const float p4 = 1.6666665459e-1f;
    const float p5 = 5.0000001201e-1f;
    float poly = p0;
    poly = (poly * r) + p1;
    poly = (poly * r) + p2;
    poly = (poly * r) + p3;
    poly = (poly * r) + p4;
    poly = (poly * r) + p5;
    poly = (poly * r * r) + r + 1.f;
    focus_math_ieee754_t u;
    u.i = ((k + 127) << 23);
    return poly * u.f;
}

float focus_math_sin(float x) {
    return focus_math_sin_eval(focus_math_angle_wrap(x));
}

float focus_math_cos(float x) {
    return focus_math_sin_eval(focus_math_angle_wrap(x + FOCUS_HALF_PI));
}

float focus_math_atan2(const float y, const float x) {
    const float abs_y = ((y >= 0.f) ? y : -y) + 1e-10f;
    float r;
    float angle;
    if(x < 0.f) {
        r = (x + abs_y) / (abs_y - x);
        angle = FOCUS_3QUARTER_PI;
    } else {
        r = (x - abs_y) / (x + abs_y);
        angle = FOCUS_QUARTER_PI;
    }
    angle += (((0.1963f * r * r) - 0.9817f) * r);
    return ((y >= 0.f) ? angle : -angle);
}

float focus_math_clamp(const float x, const float min, const float max) {
    return ((x < min) ? min : ((x > max) ? max : x));
}

void focus_math_clamp_vector(const float in[2], const float max_len, float out[2]) {
    const float len = focus_math_sqrt((in[0] * in[0]) + (in[1] * in[1]));
    const float ratio = (len > max_len) ? (max_len / len) : 1.f;
    out[0] = ratio * in[0];
    out[1] = ratio * in[1];
}

float focus_math_angle_wrap(const float in) {
    const int32_t k = (int32_t)((in * FOCUS_INV_2PI) + ((in >= 0.f) ? 0.5f : -0.5f));
    return in - (FOCUS_2PI * ((float)k));
}

float focus_math_angle_sub(const float angle1, const float angle2) {
    const float a1 = focus_math_angle_wrap(angle1);
    const float a2 = focus_math_angle_wrap(angle2);
    return focus_math_angle_wrap(a1 - a2);
}

int32_t focus_math_lerp(const int32_t x1,
                        const int32_t y1,
                        const int32_t x2,
                        const int32_t y2,
                        const int32_t xi) {
    if(x1 == x2) {
        return (y1 + y2) / 2;
    }

    return y1 + (((y2 - y1) * (xi - x1)) / (x2 - x1));
}

float focus_math_sign(const float in) {
    return (in >= 0.f) ? 1.f : -1.f;
}

void focus_math_clark_transform(const float i_uvw[3], float i_ab[2]) {
    i_ab[0] = i_uvw[0];
    i_ab[1] = (FOCUS_SQRT3_DIV3 * i_uvw[0]) + (2.f * FOCUS_SQRT3_DIV3 * i_uvw[1]);
}

void focus_math_park_transform(const float i_ab[2], const float theta, float i_dq[2]) {
    const float sin_theta = focus_math_sin(theta);
    const float cos_theta = focus_math_cos(theta);

    i_dq[0] = +(i_ab[0] * cos_theta) + (i_ab[1] * sin_theta);
    i_dq[1] = -(i_ab[0] * sin_theta) + (i_ab[1] * cos_theta);
}

void focus_math_inverse_park_transform(const float u_dq[2], const float theta, float u_ab[2]) {
    const float sin_theta = focus_math_sin(theta);
    const float cos_theta = focus_math_cos(theta);

    u_ab[0] = (u_dq[0] * cos_theta) - (u_dq[1] * sin_theta);
    u_ab[1] = (u_dq[0] * sin_theta) + (u_dq[1] * cos_theta);
}

void focus_math_inverse_clark_transform(const float u_ab[2], float u_uvw[3]) {
    u_uvw[0] = u_ab[0];
    u_uvw[1] = -(0.5f * u_ab[0]) + (FOCUS_SQRT3_DIV2 * u_ab[1]);
    u_uvw[2] = -(0.5f * u_ab[0]) - (FOCUS_SQRT3_DIV2 * u_ab[1]);
}

void focus_math_svpwm(const float u_ab[2], float u_supply, float duty_cycle_uvw[3]) {
    const float u_alpha = u_ab[0] / u_supply;
    const float u_beta = u_ab[1] / u_supply;

    uint8_t sector;
    if(u_beta > 0.f) {
        if(u_alpha > 0.f) {
            sector = (u_beta > (+FOCUS_SQRT3 * u_alpha)) ? 2 : 1;
        } else {
            sector = (u_beta > (-FOCUS_SQRT3 * u_alpha)) ? 2 : 3;
        }
    } else {
        if(u_alpha > 0.f) {
            sector = (u_beta > (-FOCUS_SQRT3 * u_alpha)) ? 6 : 5;
        } else {
            sector = (u_beta > (+FOCUS_SQRT3 * u_alpha)) ? 4 : 5;
        }
    }

    switch(sector) {
        case 1: {
            const float t1 = (1.5f * u_alpha) - (FOCUS_SQRT3_DIV2 * u_beta);
            const float t2 = (FOCUS_SQRT3 * u_beta);
            duty_cycle_uvw[0] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[1] = duty_cycle_uvw[0] - t1;
            duty_cycle_uvw[2] = duty_cycle_uvw[1] - t2;
        } break;
        case 2: {
            const float t1 = (+1.5f * u_alpha) + (FOCUS_SQRT3_DIV2 * u_beta);
            const float t2 = (-1.5f * u_alpha) + (FOCUS_SQRT3_DIV2 * u_beta);
            duty_cycle_uvw[1] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[0] = duty_cycle_uvw[1] - t2;
            duty_cycle_uvw[2] = duty_cycle_uvw[0] - t1;
        } break;
        case 3: {
            const float t1 = (FOCUS_SQRT3 * u_beta);
            const float t2 = (-1.5f * u_alpha) - (FOCUS_SQRT3_DIV2 * u_beta);
            duty_cycle_uvw[1] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[2] = duty_cycle_uvw[1] - t1;
            duty_cycle_uvw[0] = duty_cycle_uvw[2] - t2;
        } break;
        case 4: {
            const float t1 = (-1.5f * u_alpha) + (FOCUS_SQRT3_DIV2 * u_beta);
            const float t2 = (-FOCUS_SQRT3 * u_beta);
            duty_cycle_uvw[2] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[1] = duty_cycle_uvw[2] - t2;
            duty_cycle_uvw[0] = duty_cycle_uvw[1] - t1;
        } break;
        case 5: {
            const float t1 = (-1.5f * u_alpha) - (FOCUS_SQRT3_DIV2 * u_beta);
            const float t2 = (+1.5f * u_alpha) - (FOCUS_SQRT3_DIV2 * u_beta);
            duty_cycle_uvw[2] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[0] = duty_cycle_uvw[2] - t1;
            duty_cycle_uvw[1] = duty_cycle_uvw[0] - t2;
        } break;
        case 6: {
            const float t1 = (-FOCUS_SQRT3 * u_beta);
            const float t2 = (1.5f * u_alpha) + (FOCUS_SQRT3_DIV2 * u_beta);
            duty_cycle_uvw[0] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[2] = duty_cycle_uvw[0] - t2;
            duty_cycle_uvw[1] = duty_cycle_uvw[2] - t1;
        } break;
    }

    duty_cycle_uvw[0] = focus_math_clamp(duty_cycle_uvw[0], 0.f, 1.f);
    duty_cycle_uvw[1] = focus_math_clamp(duty_cycle_uvw[1], 0.f, 1.f);
    duty_cycle_uvw[2] = focus_math_clamp(duty_cycle_uvw[2], 0.f, 1.f);
}

void focus_math_dft(const float *signal,
                    const uint32_t signal_length,
                    const float signal_sample_period,
                    const float target_frequency,
                    float *amplitude,
                    float *phase,
                    float *bias) {

    float mean = 0.f;
    for(uint32_t i = 0; i < signal_length; i++) {
        mean += signal[i];
    }
    mean /= signal_length;

    const float omega = FOCUS_2PI * target_frequency * signal_sample_period;

    float real = 0.f;
    float imag = 0.f;
    for(uint32_t i = 0; i < signal_length; i++) {
        const float angle = omega * i;
        const float value = signal[i] - mean;
        real += (value * focus_math_sin(angle));
        imag += (value * focus_math_cos(angle));
    }
    real /= signal_length;
    imag /= signal_length;

    if(amplitude != NULL) {
        *amplitude = 2.f * focus_math_sqrt((real * real) + (imag * imag));
    }

    if(phase != NULL) {
        *phase = focus_math_atan2(imag, real);
    }

    if(bias != NULL) {
        *bias = mean;
    }
}

float focus_math_inverse_dft(const float amplitude, const float phase) {
    return amplitude * focus_math_cos(phase);
}

void focus_math_sdft_start(focus_math_sdft_t *sdft, float *samples, const uint32_t window) {
    sdft->samples = samples;
    sdft->index = 0;
    sdft->window = window;

    const float omega = FOCUS_2PI / sdft->window;
    sdft->rotate_real = focus_math_cos(omega);
    sdft->rotate_imag = focus_math_sin(omega);

    sdft->real = 0.f;
    sdft->imag = 0.f;
    for(uint32_t i = 0; i < sdft->window; i++) {
        sdft->samples[i] = 0.f;
    }
}

void focus_math_sdft_update(focus_math_sdft_t *sdft,
                            const float sample,
                            float *amplitude,
                            float *phase) {
    const float sample_old = sdft->samples[sdft->index];
    sdft->samples[sdft->index] = sample;

    const float real = sdft->real + (sample - sample_old);
    const float imag = sdft->imag;
    sdft->real = (sdft->rotate_real * real) - (sdft->rotate_imag * imag);
    sdft->imag = (sdft->rotate_imag * real) + (sdft->rotate_real * imag);

    if(amplitude != NULL) {
        *amplitude = 2.f * focus_math_sqrt((sdft->real * sdft->real) + (sdft->imag * sdft->imag)) /
                     sdft->window;
    }

    if(phase != NULL) {
        *phase = focus_math_atan2(sdft->imag, sdft->real);
    }

    sdft->index++;
    if(sdft->index >= sdft->window) {
        sdft->index = 0;
    }
}
