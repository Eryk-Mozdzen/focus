// gcc -I focus/include -Os -c focus/math.c
// gcc -I focus/include -Os -c tests/test.c
// gcc test.o math.o -lm

#include <math.h>
#include <stdio.h>

#include "focus/math.h"

float min(const float a, const float b) {
    return ((a < b) ? a : b);
}

float max(const float a, const float b) {
    return ((a > b) ? a : b);
}

float wrap(float in) {
    while(in > M_PI) {
        in -= (2 * M_PI);
    }
    while(in < (-M_PI)) {
        in += (2 * M_PI);
    }
    return in;
}

int main() {
    {
        float x = 0;
        float abs_error = 0;
        while(x < 100) {
            abs_error = max(abs_error, fabs(fabs(x) - focus_math_abs(x)));
            x += 1e-5f;
        }
        printf("abs   max error is %f\n\r", abs_error);
    }

    {
        float x = 0;
        float sqrt_error = 0;
        while(x < 1000) {
            sqrt_error = max(sqrt_error, fabs(sqrtf(x) - focus_math_sqrt(x)));
            x += 1e-4f;
        }
        printf("sqrt  max error is %f\n\r", sqrt_error);
    }

    {
        float x = -87;
        float exp_error = 0;
        while(x < 10) {
            exp_error = max(exp_error, fabs(expf(x) - focus_math_exp(x)));
            x += 1e-5f;
        }
        printf("exp   max error is %f\n\r", exp_error);
    }

    {
        float x = -100;
        float sin_error = 0;
        float cos_error = 0;
        while(x < 100) {
            sin_error = max(sin_error, fabs(sinf(x) - focus_math_sin(x)));
            cos_error = max(cos_error, fabs(cosf(x) - focus_math_cos(x)));
            x += 1e-5f;
        }
        printf("sin   max error is %f\n\r", sin_error);
        printf("cos   max error is %f\n\r", cos_error);
    }

    {
        float x = -100;
        float y = -100;
        float atan2_error = 0;
        while(x < 100) {
            while(y < 100) {
                atan2_error = max(atan2_error, fabs(atan2f(y, x) - focus_math_atan2(y, x)));
                y += 1e-4f;
            }
            x += 1e-4f;
        }
        printf("atan2 max error is %f deg\n\r", ((double)atan2_error) * 180. / M_PI);
    }

    {
        float x = -1000;
        float wrap_error = 0;
        float wrap_min = 0;
        float wrap_max = 0;
        while(x < 1000) {
            wrap_min = min(wrap_min, focus_math_angle_wrap(x));
            wrap_max = max(wrap_max, focus_math_angle_wrap(x));
            wrap_error = max(wrap_error, fabs(wrap(wrap(x) - focus_math_angle_wrap(x))));
            x += 1e-4f;
        }
        printf("wrap  min value is %f deg\n\r", ((double)wrap_min) * 180. / M_PI);
        printf("wrap  max value is %f deg\n\r", ((double)wrap_max) * 180. / M_PI);
        printf("wrap  max error is %f deg\n\r", ((double)wrap_error) * 180. / M_PI);
    }

    return 0;
}
