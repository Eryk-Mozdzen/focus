#ifndef FOCUS_COMMON_H
#define FOCUS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define focus_container_of(ptr, type, member)                                                      \
    ((type *)((unsigned char *)(ptr) - offsetof(type, member)))

#define focus_send_event(entity, event)                                                            \
    do {                                                                                           \
        if((entity) != NULL) {                                                                     \
            if((entity)->driver != NULL) {                                                         \
                (entity)->driver(entity, event);                                                   \
            }                                                                                      \
        }                                                                                          \
    } while(0)

enum focus_event_type {
    FOCUS_EVENT_TYPE_SRV_CONTROL_INIT,
    FOCUS_EVENT_TYPE_SRV_CONTROL_CALIBRATE,
    FOCUS_EVENT_TYPE_SRV_CONTROL_START,
    FOCUS_EVENT_TYPE_SRV_CONTROL_LOOP,
    FOCUS_EVENT_TYPE_SRV_CONTROL_TASK,
    FOCUS_EVENT_TYPE_SRV_CONTROL_STOP,
    FOCUS_EVENT_TYPE_SRV_POSITION_SAMPLE,
    FOCUS_EVENT_TYPE_SRV_INVERTER_SAMPLE,
    FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_START,
    FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_LOOP,
    FOCUS_EVENT_TYPE_SRV_INVERTER_CALIBRATION_ENDED,
    FOCUS_EVENT_TYPE_PORT_POSITION_SAMPLE,
    FOCUS_EVENT_TYPE_PORT_INVERTER_SYNC,
    FOCUS_EVENT_TYPE_PORT_INVERTER_SAMPLE,
};

struct focus_event {
    enum focus_event_type type;
    union {
        struct {
            float rs;
            float ld;
            float lq;
        } srv_control_start;
        struct {
            float i_ab[2];
            float i_dq[2];
            float u_dq[2];
            float u_ab[2];
            float pwm[3];
        } srv_control_loop;
        struct {
            float position_electrical;
            float velocity_electrical;
            float position_mechanical;
            float velocity_mechanical;
        } srv_position_sample;
        struct {
            float current_u;
            float current_v;
            float current_w;
            float voltage_vbus;
        } srv_inverter_sample;
        struct {
            float pwm[3];
        } srv_inverter_calibration_loop;
        struct {
            uint32_t encoder_count;
            bool encoder_index;
        } port_position_sample;
        struct {
            float current_u;
            float current_v;
            float current_w;
            float voltage_vbus;
        } port_inverter_sample;
    } arg;
};

struct focus_srv_control;
struct focus_srv_position;
struct focus_srv_inverter;
struct focus_port_position;
struct focus_port_inverter;

struct focus_srv_control {
    void (*driver)(struct focus_srv_control *, const struct focus_event *);
    struct focus_srv_position *position;
    struct focus_srv_inverter *inverter;
};

struct focus_srv_position {
    void (*driver)(struct focus_srv_position *, const struct focus_event *);
    struct focus_srv_control *control;
    struct focus_srv_inverter *inverter;
    struct focus_port_position *port;
};

struct focus_srv_inverter {
    void (*driver)(struct focus_srv_inverter *, const struct focus_event *);
    struct focus_srv_control *control;
    struct focus_port_inverter *port;
};

struct focus_port_position {
    void (*driver)(struct focus_port_position *, const struct focus_event *);
    struct focus_srv_position *srv;
};

struct focus_port_inverter {
    void (*driver)(struct focus_port_inverter *, const struct focus_event *);
    struct focus_srv_inverter *srv;
};

#ifdef __cplusplus
}
#endif

#endif
