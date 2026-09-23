#ifndef HW_PORT_H
#define HW_PORT_H

#include <focus/common.h>

struct hw_port_inverter {
    struct focus_port_inverter port;
};

void hw_port_inverter_driver(struct focus_port_inverter *port, const struct focus_event *event);

#ifdef EXAMPLE_ENCODER_ENABLE
struct hw_port_position {
    struct focus_port_position port;
};

void hw_port_position_driver(struct focus_port_position *port, const struct focus_event *event);
#endif

#endif
