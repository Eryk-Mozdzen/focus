#include "control/current_loop.h"
#include "control/pid.h"
#include "drivers/drv_encoder_as5600.h"
#include "drivers/drv_inverter.h"
#include "drivers/drv_memory_flash.h"
#include "drivers/drv_memory_ram.h"
#include "memory/memory_block.h"
#include "memory/memory_controller.h"

int main() {

    drv_inverter_t inverter = {
        .interface.driver = &inverter,
        .interface.init = drv_inverter_init,
        .interface.deinit = drv_inverter_deinit,
        .interface.set_duty_cycle = drv_inverter_set_duty_cycle,
        .interface.get_current = drv_inverter_get_current,
        .interface.get_supply = drv_inverter_get_supply,
    };

    drv_encoder_as5600_t encoder = {
        .interface.driver = &encoder,
        .interface.init = drv_encoder_as5600_init,
        .interface.deinit = drv_encoder_as5600_deinit,
        .interface.sample_start = drv_encoder_as5600_sample_start,
        .interface.sample_get = drv_encoder_as5600_sample_get,
    };

    drv_memory_ram_t tmp = {
        .interface.driver = &tmp,
        .interface.init = drv_memory_ram_init,
        .interface.read = drv_memory_ram_read,
        .interface.write = drv_memory_ram_write,
        .interface.flush = drv_memory_ram_flush,
    };

    drv_memory_flash_t nvm = {
        .interface.driver = &nvm,
        .interface.init = drv_memory_ram_init,
        .interface.read = drv_memory_ram_read,
        .interface.write = drv_memory_ram_write,
        .interface.flush = drv_memory_ram_flush,
    };

    memory_controller_t memory;
    memory_controller_init(&memory);
    memory_controller_register(&memory, &tmp.interface);
    memory_controller_register(&memory, &nvm.interface);

    current_loop_t current_loop = {
        .inverter = &inverter.interface,
        .encoder = &encoder.interface,
    };

    while(1) {
    }

    return 0;
}
