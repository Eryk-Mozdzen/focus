#include <stdlib.h>

#include <stm32h5xx_hal.h>

#include <dhserver.h>
#include <tusb.h>

#include <lwip/init.h>
#include <lwip/sys.h>
#include <lwip/tcp.h>
#include <lwip/timeouts.h>
#include <netif/etharp.h>

#include <focus/foc.h>
#include <focus/inverter.h>
#include <focus/math.h>
#include <focus/smo.h>

#include "hw_port.h"

#define DEBUG_COUNT 10

uint8_t tud_network_mac_address[6];
uint32_t uid[3];

static struct netif netif_data;

typedef enum {
    CONTROL_MODE_POSITION,
    CONTROL_MODE_TORQUE,
} control_mode_t;

typedef struct {
    struct focus_foc *foc;
    control_mode_t mode;
    float setpoint_position;
    float setpoint_torque;

    char buffer[128];
    uint32_t len;

    struct tcp_pcb *debug_client;
    struct tcp_pcb *telemetry_client;
} control_t;

void SystemClock_Config();
void MX_GPIO_Init();
void MX_ICACHE_Init();
void MX_USB_PCD_Init();
void MX_TIM1_Init();
void MX_TIM2_Init();
void MX_SPI1_Init();
void MX_ADC1_Init();

typedef struct {
    uint8_t *buffer;
    uint8_t *code;
    uint32_t buffer_capacity;
    uint32_t buffer_length;
} cobs_encode_t;

static void cobs_encode_start(cobs_encode_t *cobs, void *buffer, const uint32_t buffer_capacity) {
    cobs->buffer = buffer;
    cobs->buffer_capacity = buffer_capacity;
    cobs->buffer_length = 1;
    cobs->code = &cobs->buffer[0];
    *cobs->code = 1;
}

static void cobs_encode_append(cobs_encode_t *cobs, const void *data, const uint32_t data_length) {
    const uint8_t *src = data;

    for(uint32_t i = 0; (i < data_length) && (cobs->buffer_length < cobs->buffer_capacity); i++) {
        if(src[i] != 0) {
            cobs->buffer[cobs->buffer_length] = src[i];
            cobs->buffer_length++;
            (*cobs->code)++;
        } else {
            cobs->code = &cobs->buffer[cobs->buffer_length];
            cobs->buffer_length++;
            *cobs->code = 1;
        }

        if((*cobs->code == 255) && (cobs->buffer_length < cobs->buffer_capacity)) {
            cobs->code = &cobs->buffer[cobs->buffer_length];
            cobs->buffer_length++;
            *cobs->code = 1;
        }
    }
}

static uint32_t cobs_encode_finalize(cobs_encode_t *cobs) {
    if(cobs->buffer_length < cobs->buffer_capacity) {
        cobs->buffer[cobs->buffer_length] = 0;
        cobs->buffer_length++;
    }
    return cobs->buffer_length;
}

static err_t netif_linkoutput(struct netif *netif, struct pbuf *p) {
    (void)netif;

    while(1) {
        if(!tud_ready()) {
            return ERR_USE;
        }

        if(tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0);
            return ERR_OK;
        }

        tud_task();
    }
}

static err_t netif_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

static err_t netif_initialize(struct netif *netif) {
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, netif->hwaddr_len);

    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = netif_linkoutput;
    netif->output = netif_output;

    return ERR_OK;
}

static void netif_link(struct netif *netif) {
    const bool link_up = netif_is_link_up(netif);
    tud_network_link_state(0, link_up);
}

static uint32_t telnet_parse(char *buffer, char **argv, const uint32_t argv_capacity) {
    uint32_t argc = 0;

    while(*buffer && (argc < argv_capacity)) {
        while(isspace((unsigned char)*buffer)) {
            buffer++;
        }

        if(*buffer == '\0') {
            break;
        }

        argv[argc] = buffer;
        argc++;

        while(*buffer && !isspace((unsigned char)*buffer)) {
            buffer++;
        }

        if(*buffer) {
            *buffer = '\0';
            buffer++;
        }
    }

    return argc;
}

static void telnet_transmit(struct tcp_pcb *pcb, const char *message) {
    tcp_write(pcb, message, strlen(message), TCP_WRITE_FLAG_COPY);
}

static err_t telnet_receive(void *arg, struct tcp_pcb *pcb, struct pbuf *message, err_t err) {
    control_t *control = arg;

    if(message == NULL) {
        tcp_close(pcb);
        return ERR_OK;
    }

    for(uint32_t i = 0; (i < message->len) && (control->len < sizeof(control->buffer)); i++) {
        const char byte = ((uint8_t *)message->payload)[i];

        if(byte == '\n') {
            control->buffer[control->len] = '\0';

            char *argv[16];
            const uint32_t argc = telnet_parse(control->buffer, argv, 16);

            if(strcmp(argv[0], "calib_curr") == 0) {
                focus_foc_req_state(control->foc, FOCUS_FOC_STATE_CALIBRATE_INVERTER);
                telnet_transmit(pcb, "OK\r\n");
            } else if(strcmp(argv[0], "calib_mot") == 0) {
                focus_foc_req_state(control->foc, FOCUS_FOC_STATE_CALIBRATE_MOTOR);
                telnet_transmit(pcb, "OK\r\n");
            } else if(strcmp(argv[0], "calib_enc") == 0) {
                focus_foc_req_state(control->foc, FOCUS_FOC_STATE_CALIBRATE_POSITION);
                telnet_transmit(pcb, "OK\r\n");
            } else if((strcmp(argv[0], "tr") == 0) && (argc == 2)) {
                control->mode = CONTROL_MODE_TORQUE;
                control->setpoint_torque = strtof(argv[1], NULL);
                focus_foc_req_state(control->foc, FOCUS_FOC_STATE_RUNNING);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "    torque setpoint = %f Nm\n\rOK\n\r",
                         control->setpoint_torque);
                telnet_transmit(pcb, buffer);
            } else if((strcmp(argv[0], "pos") == 0) && (argc == 2)) {
                control->mode = CONTROL_MODE_POSITION;
                control->setpoint_position = focus_math_angle_wrap(strtof(argv[1], NULL));
                focus_foc_req_state(control->foc, FOCUS_FOC_STATE_RUNNING);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "    pos setpoint = %f rad\n\rOK\n\r",
                         control->setpoint_position);
                telnet_transmit(pcb, buffer);
            } else if(strcmp(argv[0], "stop") == 0) {
                control->setpoint_position = 0.f;
                control->setpoint_torque = 0.f;
                focus_foc_req_state(control->foc, FOCUS_FOC_STATE_IDLE);
                telnet_transmit(pcb, "OK\r\n");
            }

            control->len = 0;
        }

        if((byte != '\n') && (byte != '\r')) {
            control->buffer[control->len] = byte;
            control->len++;
        }
    }

    tcp_recved(pcb, message->tot_len);
    pbuf_free(message);

    return ERR_OK;
}

static err_t telnet_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)err;

    tcp_arg(pcb, arg);
    tcp_recv(pcb, telnet_receive);

    const char *header = "------------------------------------\n\r     FOCUS "__DATE__
                         " "__TIME__
                         "\r\n------------------------------------\n\r";
    tcp_write(pcb, header, strlen(header), TCP_WRITE_FLAG_COPY);

    return ERR_OK;
}

static err_t debug_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)err;
    control_t *control = arg;
    control->debug_client = pcb;
    return ERR_OK;
}

static err_t telemetry_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)err;
    control_t *control = arg;
    control->telemetry_client = pcb;
    return ERR_OK;
}

sys_prot_t sys_arch_protect() {
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval) {
    (void)pval;
}

uint32_t sys_now() {
    return HAL_GetTick();
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    struct netif *netif = &netif_data;

    if(size) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

        if(p == NULL) {
            return false;
        }

        pbuf_take(p, src, size);

        if(netif->input(p, netif) != ERR_OK) {
            pbuf_free(p);
        }

        tud_network_recv_renew();
    }

    return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    (void)arg;

    struct pbuf *p = ref;

    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

void USB_DRD_FS_IRQHandler() {
    tud_int_handler(0);
}

int main() {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ICACHE_Init();
    MX_USB_PCD_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_SPI1_Init();
    MX_ADC1_Init();

#ifdef EXAMPLE_ENCODER_ENABLE
    struct hw_port_position port_position = {0};
    port_position.port.driver = hw_port_position_driver;
#endif

    struct hw_port_inverter port_inverter = {0};
    port_inverter.port.driver = hw_port_inverter_driver;

    struct focus_inverter inverter = {0};
    inverter.srv.driver = focus_inverter_driver;
    inverter.srv.port = &port_inverter.port;
    inverter.params.offset[0] = 0.f;
    inverter.params.offset[1] = 0.f;
    inverter.params.offset[2] = 0.f;
    inverter.params.scale[0] = 1.f;
    inverter.params.scale[1] = 1.f;
    inverter.params.scale[2] = 1.f;

#ifdef EXAMPLE_ENCODER_ENABLE

#else
    struct focus_smo smo = {0};
    smo.srv.driver = focus_smo_driver;
    smo.srv.inverter = &inverter.srv;
    smo.srv.port = NULL;
    smo.params.observer.g = 0.5f;
    smo.params.observer.eta = 1.f;
    smo.params.ramp.align_time = 0.25f;
    smo.params.ramp.align_voltage = 2.f;
    smo.params.ramp.duration = 1.f;
    smo.params.ramp.voltage = 2.f;
    smo.params.ramp.time_constant = 0.1f;
    smo.params.ramp.velocity = 200.f;
    smo.params.filter_bandwidth = 50.f;
    smo.params.sampling_frequency = 25000.f;
#endif

    struct focus_foc foc = {0};
    foc.srv.driver = focus_foc_driver;
    foc.srv.inverter = &inverter.srv;
    foc.srv.position = &smo.srv;
    foc.params.kv = 750.f;
    foc.params.npp = 7;
    foc.params.rs = 1.f;
    foc.params.ld = 1.f;
    foc.params.lq = 1.f;
    foc.params.enable_identification = true;

    focus_foc_init(&foc);

    HAL_ICACHE_Disable();
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    HAL_ICACHE_Enable();

    tud_network_mac_address[0] = ((uint8_t *)uid)[0];
    tud_network_mac_address[1] = ((uint8_t *)uid)[1];
    tud_network_mac_address[2] = ((uint8_t *)uid)[2];
    tud_network_mac_address[3] = ((uint8_t *)uid)[3];
    tud_network_mac_address[4] = ((uint8_t *)uid)[4];
    tud_network_mac_address[5] = ((uint8_t *)uid)[5];

    const tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };

    tusb_init(0, &dev_init);

    lwip_init();

    struct netif *netif = &netif_data;

    const ip4_addr_t ipaddr = IPADDR4_INIT_BYTES(192, 168, 8, 1);
    const ip4_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);

    netif_add(netif, &ipaddr, &netmask, NULL, NULL, netif_initialize, ethernet_input);
    netif_set_default(netif);
    netif_set_link_callback(netif, netif_link);
    netif_set_link_up(netif);
    while(!netif_is_up(netif)) {
    }

    dhcp_entry_t dhcp_entries[] = {
        {{0}, IPADDR4_INIT_BYTES(192, 168, 8, 2), 24 * 60 * 60},
    };

    const dhcp_config_t dhcp_config = {
        .router = IPADDR4_INIT_BYTES(0, 0, 0, 0),
        .port = 67,
        .dns = IPADDR4_INIT_BYTES(0, 0, 0, 0),
        .domain = NULL,
        .entries = dhcp_entries,
        .num_entry = TU_ARRAY_SIZE(dhcp_entries),
    };

    while(dhserv_init(&dhcp_config) != ERR_OK) {
    }

    control_t control = {0};
    control.foc = &foc;

    struct tcp_pcb *telnet_pcb = tcp_new();
    tcp_bind(telnet_pcb, IP_ADDR_ANY, 23);
    telnet_pcb = tcp_listen(telnet_pcb);
    tcp_arg(telnet_pcb, &control);
    tcp_accept(telnet_pcb, telnet_accept);

    struct tcp_pcb *debug_pcb = tcp_new();
    tcp_bind(debug_pcb, IP_ADDR_ANY, 8100);
    debug_pcb = tcp_listen(debug_pcb);
    tcp_arg(debug_pcb, &control);
    tcp_accept(debug_pcb, debug_accept);

    struct tcp_pcb *telemetry_pcb = tcp_new();
    tcp_bind(telemetry_pcb, IP_ADDR_ANY, 8200);
    telemetry_pcb = tcp_listen(telemetry_pcb);
    tcp_arg(telemetry_pcb, &control);
    tcp_accept(telemetry_pcb, telemetry_accept);

    uint32_t prev = 0;
    // uint32_t prev2 = 0;
    // uint32_t scope_transmit = 0;

    while(1) {
        const uint32_t time = HAL_GetTick();

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, ((time % 1000) < 50) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        if((time - prev) >= 100) {
            prev = time;

            const float position = focus_foc_get_position(&foc);
            const float velocity = focus_foc_get_velocity(&foc);
            const float voltage = focus_foc_get_voltage(&foc);
            const float current_uvw[3] = {
                0,
                // _focus_debug_buffer[_focus_debug_buffer_index].current_uvw[0],
                // _focus_debug_buffer[_focus_debug_buffer_index].current_uvw[1],
                // _focus_debug_buffer[_focus_debug_buffer_index].current_uvw[2],
            };
            const float pwm_uvw[3] = {
                0,
                // _focus_debug_buffer[_focus_debug_buffer_index].pwm_uvw[0],
                // _focus_debug_buffer[_focus_debug_buffer_index].pwm_uvw[1],
                // _focus_debug_buffer[_focus_debug_buffer_index].pwm_uvw[2],
            };

            uint8_t buffer[1024];
            cobs_encode_t cobs;
            cobs_encode_start(&cobs, buffer, sizeof(buffer));
            cobs_encode_append(&cobs, &position, sizeof(position));
            cobs_encode_append(&cobs, &velocity, sizeof(velocity));
            cobs_encode_append(&cobs, &voltage, sizeof(voltage));
            cobs_encode_append(&cobs, current_uvw, sizeof(current_uvw));
            cobs_encode_append(&cobs, pwm_uvw, sizeof(pwm_uvw));
            const uint32_t buffer_len = cobs_encode_finalize(&cobs);

            if(control.telemetry_client) {
                tcp_write(control.telemetry_client, buffer, buffer_len, TCP_WRITE_FLAG_COPY);
                tcp_output(control.telemetry_client);
            }
        }

        /*if((_focus_debug_buffer_index >= FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES) &&
           ((time - prev2) >= 10)) {
            prev2 = time;

            uint8_t buffer[1024];
            cobs_encode_t cobs;
            cobs_encode_start(&cobs, buffer, sizeof(buffer));
            cobs_encode_append(&cobs, &scope_transmit, sizeof(scope_transmit));
            cobs_encode_append(&cobs, (void *)&_focus_debug_buffer[scope_transmit],
                               DEBUG_COUNT * sizeof(focus_debug_t));
            const uint32_t buffer_len = cobs_encode_finalize(&cobs);

            if(control.debug_client) {
                tcp_write(control.debug_client, buffer, buffer_len, TCP_WRITE_FLAG_COPY);
                tcp_output(control.debug_client);
            }

            scope_transmit += DEBUG_COUNT;

            if(scope_transmit >= FOCUS_CONFIG_DEBUG_BUFFER_SAMPLES) {
                _focus_debug_buffer_index = 0;
                scope_transmit = 0;
            }
        }*/

        switch(control.mode) {
            case CONTROL_MODE_POSITION: {
                const float e =
                    focus_math_angle_sub(control.setpoint_position, focus_foc_get_position(&foc));
                const float de = -focus_foc_get_velocity(&foc);

                const float kp = 0.01f;
                const float kd = 0.0002f;

                const float u = (kp * e) + (kd * de);

                focus_foc_set_torque(&foc, focus_math_clamp(u, -0.03f, 0.03f));
            } break;
            case CONTROL_MODE_TORQUE: {
                focus_foc_set_torque(&foc, control.setpoint_torque);
            } break;
        }

        tud_task();
        sys_check_timeouts();
        focus_foc_task(&foc);
    }

    return 0;
}
