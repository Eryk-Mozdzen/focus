#include "control/current_loop.h"
#include "control/pid.h"
#include "drivers/drv_encoder_as5600.h"
#include "drivers/drv_inverter.h"

int main() {

    drv_inverter_t inverter = {
        .interface.driver = &inverter,
        .interface.init = drv_inverter_init,
        .interface.deinit = drv_inverter_deinit,
        .interface.set_duty_cycle = drv_inverter_set_duty_cycle,
        .interface.get_current = drv_inverter_get_current,
    };

    drv_encoder_as5600_t encoder = {
        .interface.driver = &encoder,
        .interface.init = drv_encoder_as5600_init,
        .interface.deinit = drv_encoder_as5600_deinit,
        .interface.sample_start = drv_encoder_as5600_sample_start,
        .interface.sample_get = drv_encoder_as5600_sample_get,
    };

    current_loop_t current_loop = {
        .inverter = &inverter.interface,
        .encoder = &encoder.interface,
    };

    current_loop_start(&current_loop);
    current_loop_set_current_setpoint(&current_loop, 0.1);

    while(1) {
    }

    return 0;
}
