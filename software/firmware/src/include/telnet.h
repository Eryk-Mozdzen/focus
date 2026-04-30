#ifndef TELNET_H
#define TELNET_H

#include <stdint.h>

#include <lwip/tcp.h>

struct telnet_writer;

typedef struct telnet_writer telnet_writer_t;

typedef void (*telnet_callback_t)(const char *, telnet_writer_t *, void *);

typedef struct {
    struct tcp_pcb *pcb;
    char buffer[128];
    uint32_t len;

    telnet_callback_t callback;
    void *user;
} telnet_client_t;

void telnet_init(telnet_client_t *client, const telnet_callback_t callback, void *user);
void telnet_write(telnet_writer_t *writer, const char *message);

#endif
