cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "toolchain.cmake")

project(firmware)

enable_language(ASM C)

add_executable(${CMAKE_PROJECT_NAME}.elf
    "src/main.c"
    "src/freertos.c"

    "FreeRTOS-Kernel/croutine.c"
    "FreeRTOS-Kernel/event_groups.c"
    "FreeRTOS-Kernel/list.c"
    "FreeRTOS-Kernel/queue.c"
    "FreeRTOS-Kernel/stream_buffer.c"
    "FreeRTOS-Kernel/tasks.c"
    "FreeRTOS-Kernel/timers.c"
    "FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.c"
    "FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/port.c"
    "FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c"

    "cubemx/Core/Src/main.c"
    "cubemx/Core/Src/stm32h5xx_hal_msp.c"
    "cubemx/Core/Src/stm32h5xx_hal_timebase_tim.c"
    "cubemx/Core/Src/stm32h5xx_it.c"
    "cubemx/Core/Src/syscalls.c"
    "cubemx/Core/Src/sysmem.c"
    "cubemx/Core/Src/system_stm32h5xx.c"
    "cubemx/startup_stm32h523xx.s"

    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_adc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_adc_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cec.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_comp.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cordic.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cortex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_crc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_crc_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cryp.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cryp_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dac.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dac_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dcache.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dcmi.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dma.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dma_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dts.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_eth.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_eth_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_exti.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_fdcan.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_flash.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_flash_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_fmac.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_gpio.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_gtzc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_hash.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_hcd.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i2c.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i2c_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i2s.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i2s_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i3c.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_icache.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_irda.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_iwdg.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_lptim.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_mmc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_mmc_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_nand.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_nor.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_opamp.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_opamp_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_otfdec.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pcd.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pcd_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pka.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pssi.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pwr.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pwr_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_ramcfg.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rcc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rcc_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rng.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rng_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rtc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rtc_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sai.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sai_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sd.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sd_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sdio.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sdram.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_smartcard.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_smartcard_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_smbus.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_smbus_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_spi.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_spi_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_sram.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_tim.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_tim_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_uart.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_uart_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_usart.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_usart_ex.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_wwdg.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_xspi.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_adc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_comp.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_cordic.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_crc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_crs.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_dac.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_dlyb.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_dma.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_exti.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_fmac.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_fmc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_gpio.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_i2c.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_i3c.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_icache.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_lptim.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_lpuart.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_opamp.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_pka.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_pwr.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_rcc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_rng.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_rtc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_sdmmc.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_spi.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_tim.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_ucpd.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_usart.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_usb.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_ll_utils.c"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_util_i3c.c"
)

target_include_directories(${CMAKE_PROJECT_NAME}.elf PRIVATE
	"src/include"
    "FreeRTOS-Kernel/include"
    "FreeRTOS-Kernel/portable/GCC/ARM_CM33_NTZ/non_secure"
    "cubemx/Core/Inc"
    "cubemx/Drivers/STM32H5xx_HAL_Driver/Inc"
    "cubemx/Drivers/CMSIS/Include"
    "cubemx/Drivers/CMSIS/Device/ST/STM32H5xx/Include"
)

target_compile_definitions(${CMAKE_PROJECT_NAME}.elf PRIVATE
    USE_HAL_DRIVER
    USE_FULL_LL_DRIVER
    STM32H523xx
)

target_compile_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
    $<$<COMPILE_LANGUAGE:ASM>:
        -x assembler-with-cpp

        -mthumb
        -mcpu=cortex-m33
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
	>

	$<$<COMPILE_LANGUAGE:C>:
		-Os
		-std=c23
		-Wall
        -Wextra
        -Wpedantic

        -Wno-unused-parameter

		-ffunction-sections
		-fdata-sections

        -mthumb
		-mcpu=cortex-m33
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
	>
)

target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
	-T ${CMAKE_SOURCE_DIR}/cubemx/STM32H523xx_FLASH.ld
	-Wl,-Map=${CMAKE_PROJECT_NAME}.map
	-Wl,--gc-sections
	-Wl,-cref
    -Wl,--start-group -lc -lm -Wl,--end-group

    --specs=nano.specs

    -mthumb
	-mcpu=cortex-m33
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -j .text -D ${CMAKE_PROJECT_NAME}.elf > ${CMAKE_PROJECT_NAME}.dump
	COMMAND ${CMAKE_SIZE} ${CMAKE_PROJECT_NAME}.elf
)
