#include <stm32h5xx_hal.h>

#include <dhserver.h>
#include <tusb.h>

#include <lwip/apps/mqtt.h>
#include <lwip/init.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>

#include <focus/api.h>

#include "msgpack.h"

#define INIT_IP4(a, b, c, d) {PP_HTONL(LWIP_MAKEU32(a, b, c, d))}

uint8_t tud_network_mac_address[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};

static struct netif netif_data;

void SystemClock_Config();
void MX_GPIO_Init();
void MX_USB_PCD_Init();

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

    if(!msgpack_read_float32(&msgpack, rapid)) {
        return;
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
    MX_USB_PCD_Init();

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
    mqtt_subscribe(mqtt_client, "test/rapid", 0, NULL, NULL);

    focus_context_t focus_context;
    focus_init(&focus_context, NULL);

    uint32_t prev = 0;

    while(1) {
        const uint32_t time = HAL_GetTick();

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, ((time % 1000) < 50) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        if((time - prev) >= 10) {
            prev = time;

            const float val = sinf(2.f * 3.1415f * 0.1f * 0.001f * time) * rapid;

            uint8_t buffer[64];
            msgpack_t msgpack;
            msgpack_create_empty(&msgpack, buffer, sizeof(buffer));
            msgpack_write_map(&msgpack, 1);
            msgpack_write_str(&msgpack, "val");
            msgpack_write_float32(&msgpack, val);

            mqtt_publish(mqtt_client, "test/topic", msgpack.buffer, msgpack.size, 0, 0, NULL, NULL);
        }

        tud_task();
        sys_check_timeouts();
        focus_task();
    }

    return 0;
}
