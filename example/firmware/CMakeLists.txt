cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "toolchain.cmake")

project(firmware)

enable_language(ASM C)

add_executable(${CMAKE_PROJECT_NAME}.elf
    "src/main.c"
    "src/focus_port.c"
    "src/usb_descriptors.c"
    "src/msgpack.c"
    "src/telnet.c"

    "../../focus/biquad.c"
    "../../focus/core.c"
    "../../focus/debug.c"
    "../../focus/fsm.c"
    "../../focus/math.c"
    "../../focus/pid.c"
    "../../focus/smo.c"

    "cmp/cmp.c"

    "lwip/src/api/api_lib.c"
    "lwip/src/api/api_msg.c"
    "lwip/src/api/err.c"
    "lwip/src/api/if_api.c"
    "lwip/src/api/netbuf.c"
    "lwip/src/api/netdb.c"
    "lwip/src/api/netifapi.c"
    "lwip/src/api/sockets.c"
    "lwip/src/api/tcpip.c"
    "lwip/src/apps/altcp_tls/altcp_tls_mbedtls_mem.c"
    "lwip/src/apps/http/altcp_proxyconnect.c"
    "lwip/src/apps/http/fs.c"
    "lwip/src/apps/http/http_client.c"
    "lwip/src/apps/http/httpd.c"
    "lwip/src/apps/lwiperf/lwiperf.c"
    "lwip/src/apps/mdns/mdns_domain.c"
    "lwip/src/apps/mdns/mdns_out.c"
    "lwip/src/apps/mdns/mdns.c"
    "lwip/src/apps/mqtt/mqtt.c"
    "lwip/src/apps/netbiosns/netbiosns.c"
    "lwip/src/apps/smtp/smtp.c"
    "lwip/src/apps/snmp/snmp_asn1.c"
    "lwip/src/apps/snmp/snmp_core.c"
    "lwip/src/apps/snmp/snmp_mib2_icmp.c"
    "lwip/src/apps/snmp/snmp_mib2_interfaces.c"
    "lwip/src/apps/snmp/snmp_mib2_ip.c"
    "lwip/src/apps/snmp/snmp_mib2_snmp.c"
    "lwip/src/apps/snmp/snmp_mib2_system.c"
    "lwip/src/apps/snmp/snmp_mib2_tcp.c"
    "lwip/src/apps/snmp/snmp_mib2_udp.c"
    "lwip/src/apps/snmp/snmp_mib2.c"
    "lwip/src/apps/snmp/snmp_msg.c"
    "lwip/src/apps/snmp/snmp_netconn.c"
    "lwip/src/apps/snmp/snmp_pbuf_stream.c"
    "lwip/src/apps/snmp/snmp_raw.c"
    "lwip/src/apps/snmp/snmp_scalar.c"
    "lwip/src/apps/snmp/snmp_snmpv2_framework.c"
    "lwip/src/apps/snmp/snmp_snmpv2_usm.c"
    "lwip/src/apps/snmp/snmp_table.c"
    "lwip/src/apps/snmp/snmp_threadsync.c"
    "lwip/src/apps/snmp/snmp_traps.c"
    "lwip/src/apps/snmp/snmpv3_mbedtls.c"
    "lwip/src/apps/snmp/snmpv3.c"
    "lwip/src/apps/sntp/sntp.c"
    "lwip/src/apps/tftp/tftp.c"
    "lwip/src/core/ipv4/acd.c"
    "lwip/src/core/ipv4/autoip.c"
    "lwip/src/core/ipv4/dhcp.c"
    "lwip/src/core/ipv4/etharp.c"
    "lwip/src/core/ipv4/icmp.c"
    "lwip/src/core/ipv4/igmp.c"
    "lwip/src/core/ipv4/ip4_addr.c"
    "lwip/src/core/ipv4/ip4_frag.c"
    "lwip/src/core/ipv4/ip4.c"
    "lwip/src/core/ipv6/dhcp6.c"
    "lwip/src/core/ipv6/ethip6.c"
    "lwip/src/core/ipv6/icmp6.c"
    "lwip/src/core/ipv6/inet6.c"
    "lwip/src/core/ipv6/ip6_addr.c"
    "lwip/src/core/ipv6/ip6_frag.c"
    "lwip/src/core/ipv6/ip6.c"
    "lwip/src/core/ipv6/mld6.c"
    "lwip/src/core/ipv6/nd6.c"
    "lwip/src/core/altcp_alloc.c"
    "lwip/src/core/altcp_tcp.c"
    "lwip/src/core/altcp.c"
    "lwip/src/core/def.c"
    "lwip/src/core/dns.c"
    "lwip/src/core/inet_chksum.c"
    "lwip/src/core/init.c"
    "lwip/src/core/ip.c"
    "lwip/src/core/mem.c"
    "lwip/src/core/memp.c"
    "lwip/src/core/netif.c"
    "lwip/src/core/pbuf.c"
    "lwip/src/core/raw.c"
    "lwip/src/core/stats.c"
    "lwip/src/core/sys.c"
    "lwip/src/core/tcp_in.c"
    "lwip/src/core/tcp_out.c"
    "lwip/src/core/tcp.c"
    "lwip/src/core/timeouts.c"
    "lwip/src/core/udp.c"
    "lwip/src/netif/ppp/polarssl/arc4.c"
    "lwip/src/netif/ppp/polarssl/des.c"
    "lwip/src/netif/ppp/polarssl/md4.c"
    "lwip/src/netif/ppp/polarssl/md5.c"
    "lwip/src/netif/ppp/polarssl/sha1.c"
    "lwip/src/netif/ppp/auth.c"
    "lwip/src/netif/ppp/ccp.c"
    "lwip/src/netif/ppp/chap_ms.c"
    "lwip/src/netif/ppp/chap-md5.c"
    "lwip/src/netif/ppp/chap-new.c"
    "lwip/src/netif/ppp/demand.c"
    "lwip/src/netif/ppp/eap.c"
    "lwip/src/netif/ppp/ecp.c"
    "lwip/src/netif/ppp/eui64.c"
    "lwip/src/netif/ppp/fsm.c"
    "lwip/src/netif/ppp/ipcp.c"
    "lwip/src/netif/ppp/ipv6cp.c"
    "lwip/src/netif/ppp/lcp.c"
    "lwip/src/netif/ppp/magic.c"
    "lwip/src/netif/ppp/mppe.c"
    "lwip/src/netif/ppp/multilink.c"
    "lwip/src/netif/ppp/ppp.c"
    "lwip/src/netif/ppp/pppapi.c"
    "lwip/src/netif/ppp/pppcrypt.c"
    "lwip/src/netif/ppp/pppoe.c"
    "lwip/src/netif/ppp/pppol2tp.c"
    "lwip/src/netif/ppp/pppos.c"
    "lwip/src/netif/ppp/upap.c"
    "lwip/src/netif/ppp/utils.c"
    "lwip/src/netif/ppp/vj.c"
    "lwip/src/netif/bridgeif_fdb.c"
    "lwip/src/netif/bridgeif.c"
    "lwip/src/netif/ethernet.c"
    "lwip/src/netif/lowpan6_ble.c"
    "lwip/src/netif/lowpan6_common.c"
    "lwip/src/netif/lowpan6.c"
    "lwip/src/netif/slipif.c"
    "lwip/src/netif/zepif.c"

    "tinyusb/src/class/audio/audio_device.c"
    "tinyusb/src/class/bth/bth_device.c"
    "tinyusb/src/class/cdc/cdc_device.c"
    "tinyusb/src/class/cdc/cdc_host.c"
    "tinyusb/src/class/cdc/cdc_rndis_host.c"
    "tinyusb/src/class/dfu/dfu_device.c"
    "tinyusb/src/class/dfu/dfu_rt_device.c"
    "tinyusb/src/class/hid/hid_device.c"
    "tinyusb/src/class/hid/hid_host.c"
    "tinyusb/src/class/midi/midi_device.c"
    "tinyusb/src/class/midi/midi_host.c"
    "tinyusb/src/class/msc/msc_device.c"
    "tinyusb/src/class/msc/msc_host.c"
    "tinyusb/src/class/mtp/mtp_device.c"
    "tinyusb/src/class/net/ecm_rndis_device.c"
    "tinyusb/src/class/net/ncm_device.c"
    "tinyusb/src/class/usbtmc/usbtmc_device.c"
    "tinyusb/src/class/vendor/vendor_device.c"
    "tinyusb/src/class/vendor/vendor_host.c"
    "tinyusb/src/class/video/video_device.c"
    "tinyusb/src/common/tusb_fifo.c"
    "tinyusb/src/device/usbd_control.c"
    "tinyusb/src/device/usbd.c"
    "tinyusb/src/host/hub.c"
    "tinyusb/src/host/usbh.c"
    "tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c"
    "tinyusb/src/typec/usbc.c"
    "tinyusb/src/tusb.c"
    "tinyusb/lib/networking/dhserver.c"
    "tinyusb/lib/networking/dnserver.c"
    "tinyusb/lib/networking/rndis_reports.c"

    "cubemx/Core/Src/main.c"
    "cubemx/Core/Src/stm32h5xx_hal_msp.c"
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
    "../../focus/include"
    "cmp"
    "lwip/src/include"
    "tinyusb/src"
    "tinyusb/lib/networking"
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
		-O2 -g
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

    -u _printf_float
    --specs=nano.specs

    -mthumb
	-mcpu=cortex-m33
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf PRIVATE
    m
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -j .text -D ${CMAKE_PROJECT_NAME}.elf > ${CMAKE_PROJECT_NAME}.dump
	COMMAND ${CMAKE_SIZE} ${CMAKE_PROJECT_NAME}.elf
)
