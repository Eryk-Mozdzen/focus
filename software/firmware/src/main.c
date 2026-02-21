#include <FreeRTOS.h>
#include <task.h>

#include <stm32h5xx_hal.h>

void SystemClock_Config();
void MX_GPIO_Init();

static StaticTask_t task;
static StackType_t stack[1024];

static void function(void *params) {
    while(1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
        vTaskDelay(500);
    }
}

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    xTaskCreateStatic(function, "blink", 1024, NULL, 1, stack, &task);

    vTaskStartScheduler();

    return 0;
}
