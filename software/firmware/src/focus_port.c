#include <focus/port.h>
#include <stm32h5xx_hal.h>

void focus_port_init(void *user) {
}

void focus_port_start(void *user) {
}

void focus_port_shutdown(void *user) {
}

void focus_port_control(const focus_port_control_t *control, void *user) {
}

float focus_port_timebase(void *user) {
    (void)user;
    return 0.001f * HAL_GetTick();
}
