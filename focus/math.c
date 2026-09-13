#include <stddef.h>
#include <stdint.h>

#include "focus/math.h"

#define FOCUS_MATH_LOOKUP_SIN_NUM 256

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

static const float focus_math_lookup_sin[FOCUS_MATH_LOOKUP_SIN_NUM + 1] = {
    0.0f,          0.00613588465f, 0.0122715383f, 0.0184067299f, 0.0245412285f, 0.0306748032f,
    0.0368072229f, 0.0429382569f,  0.0490676743f, 0.0551952443f, 0.0613207363f, 0.0674439196f,
    0.0735645636f, 0.079682438f,   0.0857973123f, 0.0919089565f, 0.0980171403f, 0.104121634f,
    0.110222207f,  0.116318631f,   0.122410675f,  0.128498111f,  0.134580709f,  0.140658239f,
    0.146730474f,  0.152797185f,   0.158858143f,  0.16491312f,   0.170961889f,  0.17700422f,
    0.183039888f,  0.189068664f,   0.195090322f,  0.201104635f,  0.207111376f,  0.21311032f,
    0.21910124f,   0.225083911f,   0.231058108f,  0.237023606f,  0.24298018f,   0.248927606f,
    0.25486566f,   0.260794118f,   0.266712757f,  0.272621355f,  0.278519689f,  0.284407537f,
    0.290284677f,  0.296150888f,   0.302005949f,  0.30784964f,   0.31368174f,   0.319502031f,
    0.325310292f,  0.331106306f,   0.336889853f,  0.342660717f,  0.34841868f,   0.354163525f,
    0.359895037f,  0.365612998f,   0.371317194f,  0.37700741f,   0.382683432f,  0.388345047f,
    0.39399204f,   0.3996242f,     0.405241314f,  0.410843171f,  0.41642956f,   0.422000271f,
    0.427555093f,  0.433093819f,   0.438616239f,  0.444122145f,  0.44961133f,   0.455083587f,
    0.460538711f,  0.465976496f,   0.471396737f,  0.47679923f,   0.482183772f,  0.48755016f,
    0.492898192f,  0.498227667f,   0.503538384f,  0.508830143f,  0.514102744f,  0.51935599f,
    0.524589683f,  0.529803625f,   0.53499762f,   0.540171473f,  0.545324988f,  0.550457973f,
    0.555570233f,  0.560661576f,   0.565731811f,  0.570780746f,  0.575808191f,  0.580813958f,
    0.585797857f,  0.590759702f,   0.595699304f,  0.600616479f,  0.605511041f,  0.610382806f,
    0.615231591f,  0.620057212f,   0.624859488f,  0.629638239f,  0.634393284f,  0.639124445f,
    0.643831543f,  0.648514401f,   0.653172843f,  0.657806693f,  0.662415778f,  0.666999922f,
    0.671558955f,  0.676092704f,   0.680600998f,  0.685083668f,  0.689540545f,  0.693971461f,
    0.698376249f,  0.702754744f,   0.707106781f,  0.711432196f,  0.715730825f,  0.720002508f,
    0.724247083f,  0.72846439f,    0.732654272f,  0.736816569f,  0.740951125f,  0.745057785f,
    0.749136395f,  0.753186799f,   0.757208847f,  0.761202385f,  0.765167266f,  0.769103338f,
    0.773010453f,  0.776888466f,   0.780737229f,  0.784556597f,  0.788346428f,  0.792106577f,
    0.795836905f,  0.799537269f,   0.803207531f,  0.806847554f,  0.810457198f,  0.81403633f,
    0.817584813f,  0.821102515f,   0.824589303f,  0.828045045f,  0.831469612f,  0.834862875f,
    0.838224706f,  0.841554977f,   0.844853565f,  0.848120345f,  0.851355193f,  0.854557988f,
    0.85772861f,   0.860866939f,   0.863972856f,  0.867046246f,  0.870086991f,  0.873094978f,
    0.876070094f,  0.879012226f,   0.881921264f,  0.884797098f,  0.88763962f,   0.890448723f,
    0.893224301f,  0.89596625f,    0.898674466f,  0.901348847f,  0.903989293f,  0.906595705f,
    0.909167983f,  0.911706032f,   0.914209756f,  0.91667906f,   0.919113852f,  0.921514039f,
    0.923879533f,  0.926210242f,   0.92850608f,   0.930766961f,  0.932992799f,  0.93518351f,
    0.937339012f,  0.939459224f,   0.941544065f,  0.943593458f,  0.945607325f,  0.947585591f,
    0.949528181f,  0.951435021f,   0.95330604f,   0.955141168f,  0.956940336f,  0.958703475f,
    0.960430519f,  0.962121404f,   0.963776066f,  0.965394442f,  0.966976471f,  0.968522094f,
    0.970031253f,  0.971503891f,   0.972939952f,  0.974339383f,  0.97570213f,   0.977028143f,
    0.978317371f,  0.979569766f,   0.98078528f,   0.981963869f,  0.983105487f,  0.984210092f,
    0.985277642f,  0.986308097f,   0.987301418f,  0.988257568f,  0.98917651f,   0.99005821f,
    0.990902635f,  0.991709754f,   0.992479535f,  0.993211949f,  0.99390697f,   0.994564571f,
    0.995184727f,  0.995767414f,   0.996312612f,  0.996820299f,  0.997290457f,  0.997723067f,
    0.998118113f,  0.998475581f,   0.998795456f,  0.999077728f,  0.999322385f,  0.999529418f,
    0.999698819f,  0.999830582f,   0.999924702f,  0.999981175f,  1.0f,
};

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
    const float scaled = x * (((float)FOCUS_MATH_LOOKUP_SIN_NUM) / FOCUS_HALF_PI);
    int32_t idx = (int32_t)scaled;
    if(idx < 0) {
        idx = 0;
    } else if(idx >= FOCUS_MATH_LOOKUP_SIN_NUM) {
        idx = FOCUS_MATH_LOOKUP_SIN_NUM - 1;
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
