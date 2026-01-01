#include <focus.h>

static volatile int flag = 0;

void ISR() {
    flag = 1;
}

static void thread(void *user) {
    (void)user;

    if(flag) {
        flag = 0;
        // stuff
    }
}

uint32_t driver_inverter_reset(void *user) {
    (void)user;

    if(id) {
        focus_thread_destroy(id);
    }

    id = focus_thread_create(thread, NULL);

    return 0;
}

uint32_t driver_inverter_set_gates(const float u, const float v, const float w, void *user) {
    (void)u;
    (void)v;
    (void)w;
    (void)user;
    return 0;
}

uint32_t driver_inverter_get_current(float *u, float *v, float *w, void *user) {
    (void)u;
    (void)v;
    (void)w;
    (void)user;
    return 0;
}
