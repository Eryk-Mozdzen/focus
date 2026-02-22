#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_MCU           OPT_MCU_STM32H5
#define CFG_TUSB_RHPORT0_MODE  (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUSB_OS            OPT_OS_NONE
#define CFG_TUD_MAX_SPEED      OPT_MODE_FULL_SPEED
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUSB_DEBUG         0
#define CFG_TUD_ENABLED        1

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

#define CFG_TUD_ECM_RNDIS 1
#define CFG_TUD_NCM       0
#define CFG_TUD_CDC       0
#define CFG_TUD_MSC       0
#define CFG_TUD_HID       0
#define CFG_TUD_MIDI      0
#define CFG_TUD_VENDOR    0

#define CFG_TUD_NCM_IN_NTB_N         1
#define CFG_TUD_NCM_IN_NTB_MAX_SIZE  (2 * TCP_MSS + 100)
#define CFG_TUD_NCM_OUT_NTB_N        1
#define CFG_TUD_NCM_OUT_NTB_MAX_SIZE (2 * TCP_MSS + 100)

#ifndef asm
#define asm __asm__ volatile
#endif

#endif
