#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                     0
#define LWIP_PROVIDE_ERRNO         1
#define SYS_LIGHTWEIGHT_PROT       1
#define LWIP_TIMERS                1
#define MEM_ALIGNMENT              4
#define MEM_SIZE                   (16 * 1024)
#define LWIP_RAW                   0
#define LWIP_NETCONN               0
#define LWIP_SOCKET                0
#define LWIP_DHCP                  0
#define LWIP_ICMP                  1
#define LWIP_UDP                   1
#define LWIP_TCP                   1
#define LWIP_IPV4                  1
#define LWIP_IPV6                  0
#define ETH_PAD_SIZE               0
#define LWIP_IP_ACCEPT_UDP_PORT(p) ((p) == PP_NTOHS(67))

#define TCP_MSS          (1500 /*mtu*/ - 20 /*iphdr*/ - 20 /*tcphhr*/)
#define TCP_WND          (8 * TCP_MSS)
#define TCP_SND_BUF      (8 * TCP_MSS)
#define TCP_SND_QUEUELEN (4 * TCP_SND_BUF / TCP_MSS)

#define TCP_QUEUE_OOSEQ 0

#define MEMP_NUM_TCP_SEG 32
#define PBUF_POOL_SIZE   32

#define ETHARP_SUPPORT_STATIC_ENTRIES 1

#define LWIP_HTTPD_CGI             0
#define LWIP_HTTPD_SSI             0
#define LWIP_HTTPD_SSI_INCLUDE_TAG 0

#define LWIP_SINGLE_NETIF        1
#define LWIP_NETIF_LINK_CALLBACK 1

#define HTTPD_USE_CUSTOM_FSDATA 0

#define LWIP_MULTICAST_PING           1
#define LWIP_BROADCAST_PING           1
#define LWIP_IPV6_MLD                 0
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 0

#endif
