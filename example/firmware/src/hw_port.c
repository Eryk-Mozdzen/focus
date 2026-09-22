#include <stm32h5xx_hal.h>

#include <focus/common.h>

#include "hw_port.h"

#define ADC_REF     3.25f
#define ADC_RES     4096
#define SHUNT       0.001f
#define INA181_GAIN 50.f
#define INA181_REF  (0.5f * ADC_REF)
#define DIV_R1      460000.f
#define DIV_R2      49900.f

#define ADC_VOLTAGE(lsb)   ((ADC_REF * (lsb)) / (ADC_RES - 1))
#define PHASE_CURRENT(lsb) ((ADC_VOLTAGE(lsb) - INA181_REF) / (SHUNT * INA181_GAIN))
#define VBUS_VOLTAGE(lsb)  (ADC_VOLTAGE(lsb) * ((DIV_R1 + DIV_R2) / DIV_R2))

#define CLAMP(x, ARR) (((x) > ARR) ? ARR : (((x) < 0) ? 0 : (x)))

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi1;

static struct focus_port_position *port_position = NULL;
static struct focus_port_inverter *port_inverter = NULL;

#ifdef EXAMPLE_ENCODER_ENABLE
static volatile uint8_t enc_index = 0;
#ifdef EXAMPLE_ENCODER_TYPE_ABSOLUTE
static volatile uint16_t enc = 0;
static volatile uint8_t enc_ready = 1;
static uint8_t enc_buffer[3];
#endif
#endif

void hw_port_inverter_driver(struct focus_port_inverter *port, struct focus_event *event) {
    switch(event->type) {
        case FOCUS_EVENT_TYPE_SRV_CONTROL_INIT: {
            port_inverter = port;

            HAL_TIM_Base_Start(&htim1);
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
            HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
            HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
            HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
            HAL_ADCEx_InjectedStart_IT(&hadc1);
        } break;
        case FOCUS_EVENT_TYPE_SRV_CONTROL_LOOP: {
            const int32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

            const int32_t u = (1.f - event->arg.srv_control_loop.pwm[0]) * arr;
            const int32_t v = (1.f - event->arg.srv_control_loop.pwm[1]) * arr;
            const int32_t w = (1.f - event->arg.srv_control_loop.pwm[2]) * arr;

            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, CLAMP(u, arr));
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, CLAMP(v, arr));
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, CLAMP(w, arr));
        } break;
        case FOCUS_EVENT_TYPE_SRV_CONTROL_STOP: {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
        } break;
        default: {

        } break;
    }
}

void hw_port_position_driver(struct focus_port_position *port, struct focus_event *event) {
    switch(event->type) {
        case FOCUS_EVENT_TYPE_SRV_CONTROL_INIT: {
            port_position = port;

            HAL_TIM_Encoder_Start_IT(&htim2, TIM_CHANNEL_ALL);
            __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_IDX);
        } break;
        default: {

        } break;
    }
}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if(hadc == &hadc1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

#ifdef EXAMPLE_ENCODER_ENABLE
#ifdef EXAMPLE_ENCODER_TYPE_ABSOLUTE
        if(enc_ready) {
            enc_ready = 0;
            enc_buffer[0] = 0;
            enc_buffer[1] = 0;
            enc_buffer[2] = 0;
            HAL_SPI_Receive_IT(&hspi1, enc_buffer, 1);
        }
#else
        const uint16_t enc = __HAL_TIM_GET_COUNTER(&htim2);

        struct focus_event port_position_sample_event = {
            .type = FOCUS_EVENT_TYPE_PORT_POSITION_SAMPLE,
            .arg.port_postion_sample.encoder_count = enc,
            .arg.port_postion_sample.encoder_index = enc_index,
        };

        enc_index = 0;

        focus_srv_event(port_position->srv->control, &port_position_sample_event);
#endif
#endif
        const uint32_t u = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
        const uint32_t v = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
        const uint32_t w = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_3);
        const uint32_t vbus = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_4);

        struct focus_event port_inverter_sample_event = {
            .type = FOCUS_EVENT_TYPE_PORT_INVERTER_SAMPLE,
            .arg.port_inverter_sample.current_u = PHASE_CURRENT(u),
            .arg.port_inverter_sample.current_v = PHASE_CURRENT(v),
            .arg.port_inverter_sample.current_w = PHASE_CURRENT(w),
            .arg.port_inverter_sample.voltage_vbus = VBUS_VOLTAGE(vbus),
        };

        focus_srv_event(port_inverter->srv->control, &port_inverter_sample_event);

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    }
}

#ifdef EXAMPLE_ENCODER_ENABLE
#ifdef EXAMPLE_ENCODER_TYPE_ABI
void HAL_TIMEx_EncoderIndexCallback(TIM_HandleTypeDef *htim) {
    if(htim == &htim2) {
        enc_index = 1;
    }
}
#endif

#ifdef EXAMPLE_ENCODER_TYPE_ABSOLUTE
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if(hspi == &hspi1) {
        enc = (((uint16_t)enc_buffer[2]) << 6) | (((uint16_t)enc_buffer[1]) >> 2);
        enc_ready = 1;
    }
}
#endif
#endif
