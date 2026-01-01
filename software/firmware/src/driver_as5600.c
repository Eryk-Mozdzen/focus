#include <focus.h>

uint32_t driver_as5600_reset(void *user) {
    (void)user;
    return 0;
}

uint32_t driver_as5600_sample_start(void *user) {
    (void)user;
    return 0;
}

uint32_t driver_as5600_sample_get(float *sample, void *user) {
    (void)sample;
    (void)user;
    return 0;
}
