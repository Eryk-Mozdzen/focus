#include <math.h>
#include <stdint.h>

#include "focus/math.h"

float focus_math_clamp(const float x, const float min, const float max) {
    return ((x < min) ? min : ((x > max) ? max : x));
}

void focus_math_clamp_vector(const float in[2], const float max_len, float out[2]) {
    const float len = sqrtf((in[0] * in[0]) + (in[1] * in[1]));
    const float ratio = (len > max_len) ? (max_len / len) : 1.f;
    out[0] = ratio * in[0];
    out[1] = ratio * in[1];
}

float focus_math_wrap(const float in) {
    const float two_pi = 6.283185307f;
    const float inv_two_pi = 0.159154943f;
    return in - (two_pi * roundf(in * inv_two_pi));
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

void focus_math_clark_transform(const float i_uvw[3], float i_ab[2]) {
    i_ab[0] = i_uvw[0];
    i_ab[1] = (0.577350269f * i_uvw[0]) + (1.154700538f * i_uvw[1]);
}

void focus_math_park_transform(const float i_ab[2], const float theta, float i_dq[2]) {
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    i_dq[0] = +(i_ab[0] * cos_theta) + (i_ab[1] * sin_theta);
    i_dq[1] = -(i_ab[0] * sin_theta) + (i_ab[1] * cos_theta);
}

void focus_math_inverse_park_transform(const float u_dq[2], const float theta, float u_ab[2]) {
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    u_ab[0] = (u_dq[0] * cos_theta) - (u_dq[1] * sin_theta);
    u_ab[1] = (u_dq[0] * sin_theta) + (u_dq[1] * cos_theta);
}

void focus_math_inverse_clark_transform(const float u_ab[2], float u_uvw[3]) {
    u_uvw[0] = u_ab[0];
    u_uvw[1] = -(0.5f * u_ab[0]) + (0.866025404f * u_ab[1]);
    u_uvw[2] = -(0.5f * u_ab[0]) - (0.866025404f * u_ab[1]);
}

void focus_math_svpwm(const float u_ab[2], float u_supply, float duty_cycle_uvw[3]) {
    const float u_alpha = u_ab[0] / u_supply;
    const float u_beta = u_ab[1] / u_supply;

    uint8_t sector;
    if(u_beta >= 0.f) {
        if(u_alpha >= 0.f) {
            sector = ((0.577350269f * u_beta) > u_alpha) ? 2 : 1;
        } else {
            sector = ((-0.577350269f * u_beta) > u_alpha) ? 3 : 2;
        }
    } else {
        if(u_alpha >= 0.f) {
            sector = ((-0.577350269f * u_beta) > u_alpha) ? 5 : 6;
        } else {
            sector = ((0.577350269f * u_beta) > u_alpha) ? 4 : 5;
        }
    }

    switch(sector) {
        case 1: {
            const float t1 = (u_alpha - (0.577350269f * u_beta));
            const float t2 = (1.154700538f * u_beta);
            duty_cycle_uvw[0] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[1] = duty_cycle_uvw[0] - t1;
            duty_cycle_uvw[2] = duty_cycle_uvw[1] - t2;
        } break;
        case 2: {
            const float t1 = (u_alpha + (0.577350269f * u_beta));
            const float t2 = (-u_alpha + (0.577350269f * u_beta));
            duty_cycle_uvw[1] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[0] = duty_cycle_uvw[1] - t2;
            duty_cycle_uvw[2] = duty_cycle_uvw[0] - t1;
        } break;
        case 3: {
            const float t1 = (1.154700538f * u_beta);
            const float t2 = (-u_alpha - (0.577350269f * u_beta));
            duty_cycle_uvw[1] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[2] = duty_cycle_uvw[1] - t1;
            duty_cycle_uvw[0] = duty_cycle_uvw[2] - t2;
        } break;
        case 4: {
            const float t1 = (-u_alpha + (0.577350269f * u_beta));
            const float t2 = (-1.154700538f * u_beta);
            duty_cycle_uvw[2] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[1] = duty_cycle_uvw[2] - t2;
            duty_cycle_uvw[0] = duty_cycle_uvw[1] - t1;
        } break;
        case 5: {
            const float t1 = (-u_alpha - (0.577350269f * u_beta));
            const float t2 = (u_alpha - (0.577350269f * u_beta));
            duty_cycle_uvw[2] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[0] = duty_cycle_uvw[2] - t1;
            duty_cycle_uvw[1] = duty_cycle_uvw[0] - t2;
        } break;
        case 6: {
            const float t1 = (-1.154700538f * u_beta);
            const float t2 = (u_alpha + (0.577350269f * u_beta));
            duty_cycle_uvw[0] = 0.5f * (1.f + t1 + t2);
            duty_cycle_uvw[2] = duty_cycle_uvw[0] - t2;
            duty_cycle_uvw[1] = duty_cycle_uvw[2] - t1;
        } break;
    }

    duty_cycle_uvw[0] = focus_math_clamp(duty_cycle_uvw[0], 0.f, 1.f);
    duty_cycle_uvw[1] = focus_math_clamp(duty_cycle_uvw[1], 0.f, 1.f);
    duty_cycle_uvw[2] = focus_math_clamp(duty_cycle_uvw[2], 0.f, 1.f);
}
