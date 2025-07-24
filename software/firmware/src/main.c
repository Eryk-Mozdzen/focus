#include "drv_as5600.h"
#include "drv_flash.h"
#include "drv_inverter.h"
#include "drv_ram.h"
#include "focus/current_loop.h"
#include "focus/memory_controller.h"
#include "focus/memory_reg.h"
#include "focus/pid.h"

int main() {

    drv_inverter_t inverter = {
        .interface.driver = &inverter,
        .interface.init = drv_inverter_init,
        .interface.deinit = drv_inverter_deinit,
        .interface.set_duty_cycle = drv_inverter_set_duty_cycle,
        .interface.get_current = drv_inverter_get_current,
        .interface.get_supply = drv_inverter_get_supply,
    };

    drv_as5600_t encoder = {
        .interface.driver = &encoder,
        .interface.init = drv_as5600_init,
        .interface.deinit = drv_as5600_deinit,
        .interface.sample_start = drv_as5600_sample_start,
        .interface.sample_get = drv_as5600_sample_get,
    };

    current_loop_t current_loop = {
        .inverter = &inverter.interface,
        .encoder = &encoder.interface,
    };

    drv_ram_t ram = {
        .interface.driver = &ram,
        .interface.init = drv_ram_init,
        .interface.read = drv_ram_read,
        .interface.write = drv_ram_write,
        .interface.flush = drv_ram_flush,
    };

    drv_flash_t nvm = {
        .interface.driver = &nvm,
        .interface.init = drv_flash_init,
        .interface.read = drv_flash_read,
        .interface.write = drv_flash_write,
        .interface.flush = drv_flash_flush,
    };

    memory_reg_t *ram_regs[] = {
        &current_loop.current[0], &current_loop.current[1], &current_loop.current[2],
        &current_loop.supply,     &current_loop.position,
    };

    memory_reg_t *nvm_regs[] = {
        &current_loop.overcurrent,     &current_loop.overvoltage,      &current_loop.undervoltage,
        &current_loop.position_offset, &current_loop.motor_pole_pairs,
    };

    memory_controller_t memory;
    memory_controller_init(&memory);
    memory_controller_register(&memory, &ram.interface, 0x0000, ram_regs, sizeof(ram_regs));
    memory_controller_register(&memory, &nvm.interface, 0x8000, nvm_regs, sizeof(nvm_regs));

    while(1) {
    }

    return 0;
}
