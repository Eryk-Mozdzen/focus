#include <stm32h5xx_hal.h>

#include <lwip/sys.h>

sys_prot_t sys_arch_protect(void) {
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval) {
    (void)pval;
}

uint32_t sys_now(void) {
    return HAL_GetTick();
}
