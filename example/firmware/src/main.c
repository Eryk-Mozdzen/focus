#include <stdlib.h>

#include <stm32h5xx_hal.h>

#include <dhserver.h>
#include <tusb.h>

#include <lwip/apps/mqtt.h>
#include <lwip/init.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>

#include <focus/api.h>
#include <focus/math.h>

#include "msgpack.h"
#include "telnet.h"

#define INIT_IP4(a, b, c, d) {PP_HTONL(LWIP_MAKEU32(a, b, c, d))}

uint8_t tud_network_mac_address[6];
uint32_t uid[3];

volatile float debug_supply;
volatile float debug_position;
volatile float debug_position_ol;
volatile float debug_velocity;
volatile float debug_svpwm[3];
volatile float debug_ab[2];
volatile float debug_uvw[3];
volatile float scope_buffer[1000][3];
volatile uint32_t scope_index;

static struct netif netif_data;

typedef enum {
    CONTROL_MODE_POSITION,
    CONTROL_MODE_TORQUE,
} control_mode_t;

typedef struct {
    control_mode_t mode;
    float setpoint_position;
    float setpoint_torque;
} control_t;

void SystemClock_Config();
void MX_GPIO_Init();
void MX_ICACHE_Init();
void MX_USB_PCD_Init();
void MX_TIM1_Init();
void MX_TIM2_Init();
void MX_ADC1_Init();

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

static void mqtt_incoming_data(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    float *rapid = arg;

    msgpack_t msgpack;
    msgpack_create_from(&msgpack, data, len);

    uint32_t num;
    if(!msgpack_read_map(&msgpack, &num)) {
        return;
    }

    if(num != 1) {
        return;
    }

    char str[16];
    if(!msgpack_read_str(&msgpack, str, sizeof(str))) {
        return;
    }

    if(strcmp(str, "val") != 0) {
        return;
    }

    if(!msgpack_read_float32(&msgpack, rapid)) {
        return;
    }
}

static void telnet_recv(const uint32_t argc, char **argv, telnet_writer_t *writer, void *user) {
    control_t *control = user;

    if(strcmp(argv[0], "calib_curr") == 0) {
        focus_request_state(0, FOCUS_REQUESTED_STATE_CALIBRATE_CURRENT);
        telnet_write(writer, "OK\r\n");
    } else if(strcmp(argv[0], "calib_enc") == 0) {
        focus_request_state(0, FOCUS_REQUESTED_STATE_CALIBRATE_ENCODER);
        telnet_write(writer, "OK\r\n");
    } else if(strcmp(argv[0], "calib_mot") == 0) {
        focus_request_state(0, FOCUS_REQUESTED_STATE_CALIBRATE_MOTOR);
        telnet_write(writer, "OK\r\n");
    } else if((strcmp(argv[0], "iq") == 0) && (argc == 2)) {
        control->mode = CONTROL_MODE_TORQUE;
        control->setpoint_torque = strtof(argv[1], NULL);
        focus_request_state(0, FOCUS_REQUESTED_STATE_CLOSE_LOOP);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "    Iq setpoint = %f\n\rOK\n\r",
                 control->setpoint_torque);
        telnet_write(writer, buffer);
    } else if((strcmp(argv[0], "pos") == 0) && (argc == 2)) {
        control->mode = CONTROL_MODE_POSITION;
        control->setpoint_position = focus_math_angle_wrap(strtof(argv[1], NULL));
        focus_request_state(0, FOCUS_REQUESTED_STATE_CLOSE_LOOP);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "    pos setpoint = %f\n\rOK\n\r",
                 control->setpoint_position);
        telnet_write(writer, buffer);
    } else if(strcmp(argv[0], "stop") == 0) {
        control->setpoint_position = 0.f;
        control->setpoint_torque = 0.f;
        focus_request_state(0, FOCUS_REQUESTED_STATE_IDLE);
        telnet_write(writer, "OK\r\n");
    } else if(strcmp(argv[0], "calib") == 0) {
        const focus_calibration_t *data = focus_calibration_data(0);
        char buffer[256];
        snprintf(
            buffer, sizeof(buffer),
            "    Rs = %f\r\n    Ld = %f\r\n    Lq = %f\r\n    current offset = [%+6.3f, %+6.3f, "
            "%+6.3f]\n\r    current scale  = [%6.3f, %6.3f, %6.3f]\r\n",
            data->motor.rs, data->motor.ld, data->motor.lq, data->current_offset[0],
            data->current_offset[1], data->current_offset[2], data->current_scale[0],
            data->current_scale[1], data->current_scale[2]);
        telnet_write(writer, buffer);
    }
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
    MX_ADC1_Init();

    focus_init(NULL);

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

    const ip4_addr_t ipaddr = INIT_IP4(192, 168, 7, 1);
    const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
    const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

    netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_initialize, ethernet_input);
    netif_set_default(netif);
    netif_set_link_callback(netif, netif_link);
    netif_set_link_up(netif);
    while(!netif_is_up(netif)) {
    }

    dhcp_entry_t dhcp_entries[] = {
        {{0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60},
        {{0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60},
        {{0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60},
    };

    const dhcp_config_t dhcp_config = {
        .router = INIT_IP4(0, 0, 0, 0),
        .port = 67,
        .dns = INIT_IP4(0, 0, 0, 0),
        .domain = NULL,
        .entries = dhcp_entries,
        .num_entry = TU_ARRAY_SIZE(dhcp_entries),
    };

    while(dhserv_init(&dhcp_config) != ERR_OK) {
    }

    control_t control = {0};
    telnet_client_t telnet;
    telnet_init(&telnet, telnet_recv, &control);

    const ip4_addr_t mqtt_broker = INIT_IP4(192, 168, 7, 2);

    const struct mqtt_connect_client_info_t mqtt_client_info = {
        .client_id = "lwip_client",
        .client_user = NULL,
        .client_pass = NULL,
        .keep_alive = 60,
        .will_topic = NULL,
        .will_msg = NULL,
        .will_msg_len = 0,
        .will_qos = 0,
        .will_retain = 0,
    };

    float rapid = 1;

    mqtt_client_t *mqtt_client = mqtt_client_new();
    mqtt_set_inpub_callback(mqtt_client, NULL, mqtt_incoming_data, &rapid);
    mqtt_client_connect(mqtt_client, &mqtt_broker, 1883, NULL, NULL, &mqtt_client_info);
    mqtt_subscribe(mqtt_client, "focus/control", 0, NULL, NULL);

    uint32_t prev = 0;
    uint32_t prev2 = 0;
    uint32_t scope_transmit = 0;

    while(1) {
        const uint32_t time = HAL_GetTick();

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, ((time % 1000) < 50) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        if((time - prev) >= 20) {
            prev = time;

            uint8_t buffer[128];
            msgpack_t msgpack;
            msgpack_create_empty(&msgpack, buffer, sizeof(buffer));
            msgpack_write_map(&msgpack, 7);
            msgpack_write_str(&msgpack, "supply");
            msgpack_write_float32(&msgpack, debug_supply);
            msgpack_write_str(&msgpack, "position");
            msgpack_write_float32(&msgpack, debug_position);
            msgpack_write_str(&msgpack, "position_open_loop");
            msgpack_write_float32(&msgpack, debug_position_ol);
            msgpack_write_str(&msgpack, "velocity");
            msgpack_write_float32(&msgpack, debug_velocity);
            msgpack_write_str(&msgpack, "svpwm");
            msgpack_write_array(&msgpack, 3);
            msgpack_write_float32(&msgpack, debug_svpwm[0]);
            msgpack_write_float32(&msgpack, debug_svpwm[1]);
            msgpack_write_float32(&msgpack, debug_svpwm[2]);
            msgpack_write_str(&msgpack, "ab");
            msgpack_write_array(&msgpack, 2);
            msgpack_write_float32(&msgpack, debug_ab[0]);
            msgpack_write_float32(&msgpack, debug_ab[1]);
            msgpack_write_str(&msgpack, "uvw");
            msgpack_write_array(&msgpack, 3);
            msgpack_write_float32(&msgpack, debug_uvw[0]);
            msgpack_write_float32(&msgpack, debug_uvw[1]);
            msgpack_write_float32(&msgpack, debug_uvw[2]);

            mqtt_publish(mqtt_client, "focus/state", msgpack.buffer, msgpack.size, 0, 0, NULL,
                         NULL);
        }

        if((scope_index >= 1000) && ((time - prev2) >= 20)) {
            prev2 = time;

            uint8_t buffer[256];
            msgpack_t msgpack;
            msgpack_create_empty(&msgpack, buffer, sizeof(buffer));
            msgpack_write_map(&msgpack, 4);
            msgpack_write_str(&msgpack, "index");
            msgpack_write_uint32(&msgpack, scope_transmit);

            msgpack_write_str(&msgpack, "signal_1");
            msgpack_write_array(&msgpack, 10);
            for(uint32_t i = 0; i < 10; i++) {
                msgpack_write_float32(&msgpack, scope_buffer[scope_transmit + i][0]);
            }

            msgpack_write_str(&msgpack, "signal_2");
            msgpack_write_array(&msgpack, 10);
            for(uint32_t i = 0; i < 10; i++) {
                msgpack_write_float32(&msgpack, scope_buffer[scope_transmit + i][1]);
            }

            msgpack_write_str(&msgpack, "signal_3");
            msgpack_write_array(&msgpack, 10);
            for(uint32_t i = 0; i < 10; i++) {
                msgpack_write_float32(&msgpack, scope_buffer[scope_transmit + i][2]);
            }

            mqtt_publish(mqtt_client, "focus/scope", msgpack.buffer, msgpack.size, 0, 0, NULL,
                         NULL);

            scope_transmit += 10;

            if(scope_transmit >= 1000) {
                scope_index = 0;
                scope_transmit = 0;
            }
        }

        switch(control.mode) {
            case CONTROL_MODE_TORQUE: {
                focus_set_torque(0, focus_math_clamp(control.setpoint_torque, -3.f, 3.f));
            } break;
            case CONTROL_MODE_POSITION: {
                const float e =
                    focus_math_angle_sub(control.setpoint_position, focus_get_position(0));
                const float de = -focus_get_velocity(0);

                const float kp = 2.f;
                const float kd = 0.05f;

                const float u = (kp * e) + (kd * de);

                focus_set_torque(0, focus_math_clamp(u, -3.f, 3.f));
            } break;
        }

        tud_task();
        sys_check_timeouts();
        focus_task();
    }

    return 0;
}
