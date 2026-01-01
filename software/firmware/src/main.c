#include <focus.h>

#include "driver.h"

int main(void) {

    driver_flash_t driver_flash;
    const focus_memory_t memory = {
        .user = &driver_flash,
        .reset = driver_flash_reset,
        .read = driver_flash_read,
        .write = driver_flash_write,
        .flush = driver_flash_flush,
    };

    driver_inverter_t driver_inverter;
    const focus_inverter_t inverter = {
        .user = &driver_inverter,
        .reset = driver_inverter_reset,
        .set_gates = driver_inverter_set_gates,
        .get_current = driver_inverter_get_current,
    };

    driver_as5600_t driver_as5600;
    const focus_encoder_t encoder = {
        .user = &driver_as5600,
        .reset = driver_as5600_reset,
        .sample_start = driver_as5600_sample_start,
        .sample_get = driver_as5600_sample_get,
    };

    uint8_t channel_buffer[FOCUS_CHANNEL_SIZE];
    focus_channel_t channel;
    focus_channel_create(&channel, channel_buffer, &inverter, &encoder);

    uint8_t kernel_buffer[FOCUS_KERNEL_SIZE];
    focus_kernel_t kernel;
    focus_kernel_create(&kernel, kernel_buffer, &memory, &channel, 1);

    focus_kernel_start(kernel);

    while(1) {
        focus_kernel_dispatch(kernel);

        focus_channel_position_setpoint_set(channel, 1.f);
    }

    return 0;
}
