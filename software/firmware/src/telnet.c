#include <stdint.h>
#include <string.h>

#include <lwip/tcp.h>

#include "telnet.h"

struct telnet_writer {
    struct tcp_pcb *pcb;
};

static err_t telnet_receive(void *arg, struct tcp_pcb *pcb, struct pbuf *message, err_t err) {
    (void)err;

    if(message == NULL) {
        tcp_close(pcb);
        return ERR_OK;
    }

    telnet_client_t *client = arg;

    for(uint32_t i = 0; (i < message->len) && (client->len < sizeof(client->buffer)); i++) {
        const char byte = ((uint8_t *)message->payload)[i];

        if((byte == '\n') || (byte == '\r')) {
            if(client->callback != NULL) {
                client->buffer[client->len] = '\0';
                telnet_writer_t writer = {.pcb = pcb};
                client->callback(client->buffer, &writer, client->user);
            }

            break;
        }

        client->buffer[client->len] = byte;
        client->len++;
    }

    tcp_recved(pcb, message->tot_len);
    pbuf_free(message);

    return ERR_OK;
}

static err_t telnet_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    telnet_client_t *client = arg;

    client->pcb = pcb;
    client->len = 0;

    tcp_arg(pcb, client);
    tcp_recv(pcb, telnet_receive);

    const char *welcome = "FOCUS "__DATE__
                          " "__TIME__
                          "\r\n";
    tcp_write(pcb, welcome, strlen(welcome), TCP_WRITE_FLAG_COPY);

    return ERR_OK;
}

void telnet_init(telnet_client_t *client, const telnet_callback_t callback, void *user) {
    client->callback = callback;
    client->user = user;

    client->pcb = tcp_new();
    if(client->pcb) {
        tcp_bind(client->pcb, IP_ADDR_ANY, 23);

        client->pcb = tcp_listen(client->pcb);

        tcp_arg(client->pcb, client);
        tcp_accept(client->pcb, telnet_accept);
    }
}

void telnet_write(telnet_writer_t *writer, const char *message) {
    tcp_write(writer->pcb, message, strlen(message), TCP_WRITE_FLAG_COPY);
}
