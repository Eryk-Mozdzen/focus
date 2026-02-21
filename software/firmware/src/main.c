#include <FreeRTOS.h>
#include <task.h>

#include <tusb.h>

#include <stm32h5xx_hal.h>

static StaticTask_t blink_task;
static StackType_t blink_stack[256];

static StaticTask_t usbd_task;
static StackType_t usbd_stack[1024];

static StaticTask_t cdc_task;
static StackType_t cdc_stack[1024];

void SystemClock_Config();
void MX_GPIO_Init();
void MX_USB_PCD_Init();

void msc_disk_init();

static void blink(void *param) {
    (void)param;

    while(1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
        vTaskDelay(50);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
        vTaskDelay(950);
    }
}

static void usbd(void *param) {
    (void)param;

    const tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };

    tusb_init(0, &dev_init);

    msc_disk_init();

    while(1) {
        tud_task();
        tud_cdc_write_flush();
    }
}

static void cdc(void *param) {
    (void)param;

    uint32_t prev = 0;

    while(1) {
        if(tud_cdc_connected()) {
            const uint32_t now = xTaskGetTickCount();

            if((now - prev) >= 1000) {
                prev = now;
                const uint8_t str[16] = {'w', 'i', 't', 'a', 'm', '\n', '\r', '\0'};
                tud_cdc_write(str, 8);
            }

            tud_cdc_write_flush();
        }

        vTaskDelay(1);
    }
}

void USB_DRD_FS_IRQHandler() {
    tud_int_handler(0);
}

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USB_PCD_Init();

    xTaskCreateStatic(blink, "blink", 256, NULL, 1, blink_stack, &blink_task);
    xTaskCreateStatic(usbd, "usbd", 1024, NULL, 11, usbd_stack, &usbd_task);
    xTaskCreateStatic(cdc, "cdc", 1024, NULL, 10, cdc_stack, &cdc_task);

    vTaskStartScheduler();

    return 0;
}
