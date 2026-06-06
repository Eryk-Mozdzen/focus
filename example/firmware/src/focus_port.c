#include <focus/port.h>
#include <stm32h5xx_hal.h>

#define ADC_REF     3.25f
#define ADC_RES     4096
#define SHUNT       0.002f
#define INA181_GAIN 100.f
#define INA181_REF  (0.5f * ADC_REF)
#define DIV_R1      470000.f
#define DIV_R2      49900.f

#define ADC_VOLTAGE(lsb)   ((ADC_REF * (lsb)) / (ADC_RES - 1))
#define PHASE_CURRENT(lsb) ((ADC_VOLTAGE(lsb) - INA181_REF) / (SHUNT * INA181_GAIN))
#define VBUS_VOLTAGE(lsb)  (ADC_VOLTAGE(lsb) * ((DIV_R1 + DIV_R2) / DIV_R2))

#define CLAMP(x, ARR) (((x) > ARR) ? ARR : (((x) < 0) ? 0 : (x)))

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;

void focus_port_init(void *user) {
    (void)user;

    HAL_TIM_Base_Start(&htim1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
    HAL_ADCEx_InjectedStart_IT(&hadc1);

    HAL_TIM_Encoder_Start_IT(&htim2, TIM_CHANNEL_ALL);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_IDX);
}

void focus_port_start(const uint32_t motor, void *user) {
    (void)motor;
    (void)user;
}

void focus_port_shutdown(const uint32_t motor, void *user) {
    (void)motor;
    (void)user;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
}

void focus_port_control(const uint32_t motor, const focus_port_control_t *control, void *user) {
    (void)user;

    const int32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

    const int32_t u = (1.f - control->duty_cycle_u) * arr;
    const int32_t v = (1.f - control->duty_cycle_v) * arr;
    const int32_t w = (1.f - control->duty_cycle_w) * arr;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, CLAMP(u, arr));
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, CLAMP(v, arr));
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, CLAMP(w, arr));
}

float focus_port_timebase(void *user) {
    (void)user;
    return 0.001f * HAL_GetTick();
}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if(hadc == &hadc1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

#ifdef FOCUS_CONFIG_ENCODER_ABI
        const uint16_t enc = __HAL_TIM_GET_COUNTER(&htim2);
#endif
        const uint32_t u = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
        const uint32_t v = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
        const uint32_t w = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_3);
        const uint32_t vbus = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_4);

        const focus_port_sample_t sample = {
#ifdef FOCUS_CONFIG_ENCODER_ABI
            .encoder_count = enc,
#endif
            .current_u = PHASE_CURRENT(u),
            .current_v = PHASE_CURRENT(v),
            .current_w = PHASE_CURRENT(w),
            .voltage_vbus = VBUS_VOLTAGE(vbus),
        };

        focus_port_event_sample(0, &sample);

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }
}

#ifdef FOCUS_CONFIG_ENCODER_ABI
void HAL_TIMEx_EncoderIndexCallback(TIM_HandleTypeDef *htim) {
    if(htim == &htim2) {
        focus_port_event_index(0, 0);
    }
}
#endif
