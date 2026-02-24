#include <stm32h5xx_hal.h>

#include <dhserver.h>
#include <tusb.h>

#include <lwip/init.h>
#include <lwip/timeouts.h>

#include <focus/api.h>

#define INIT_IP4(a, b, c, d) {PP_HTONL(LWIP_MAKEU32(a, b, c, d))}

uint8_t tud_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};

static struct netif netif_data;

static dhcp_entry_t entries[] = {
    {{0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config = {
    .router = INIT_IP4(0, 0, 0, 0),
    .port = 67,
    .dns = INIT_IP4(192, 168, 7, 1),
    .domain = "usb",
    .entries = entries,
    .num_entry = TU_ARRAY_SIZE(entries),
};

static const ip4_addr_t ipaddr = INIT_IP4(192, 168, 7, 1);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

void SystemClock_Config();
void MX_GPIO_Init();
void MX_USB_PCD_Init();

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
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

static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

static err_t netif_init_cb(struct netif *netif) {
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = linkoutput_fn;
    netif->output = ip4_output_fn;
    return ERR_OK;
}

static void usbnet_netif_link_callback(struct netif *netif) {
    const bool link_up = netif_is_link_up(netif);
    tud_network_link_state(0, link_up);
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

    struct netif *netif = &netif_data;

    lwip_init();

    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));

    netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb, ethernet_input);
    netif_set_default(netif);

    netif_set_link_callback(netif, usbnet_netif_link_callback);
    netif_set_link_up(netif);

    while(!netif_is_up(netif)) {
    }

    while(dhserv_init(&dhcp_config) != ERR_OK) {
    }

    focus_context_t context;
    focus_init(&context, NULL);

    ip_addr_t addr;
    ipaddr_aton("192.168.7.2", &addr);
    struct udp_pcb *control = udp_new();
    uint32_t stream_prev = 0;

    while(1) {
        const uint32_t time = HAL_GetTick();

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, ((time % 1000) < 50) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        if((time - stream_prev) >= 1) {
            stream_prev = time;

            const float val = sinf(2.f * 3.1415f * 0.1f * 0.001f * time);
            char json[] = "{\"val\": xx.xxxx}";
            const uint32_t integer = fabs(val * 10000.f);
            json[8] = (val > 0) ? ' ' : '-';
            json[9] = '0' + ((integer / 10000) % 10);
            json[11] = '0' + ((integer / 1000) % 10);
            json[12] = '0' + ((integer / 100) % 10);
            json[13] = '0' + ((integer / 10) % 10);
            json[14] = '0' + ((integer / 1) % 10);

            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(json), PBUF_RAM);
            if(p) {
                memcpy(p->payload, json, strlen(json));
                udp_sendto(control, p, &addr, 5005);
                pbuf_free(p);
            }
        }

        tud_task();
        sys_check_timeouts();
        focus_task();
    }

    return 0;
}
