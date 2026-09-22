#ifndef FOCUS_COMMON_H
#define FOCUS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define focus_container_of(ptr, type, member)                                                      \
    ({                                                                                             \
        const typeof(((type *)0)->member) *__mptr = (ptr);                                         \
        (type *)((char *)__mptr - offsetof(type, member));                                         \
    })

#define focus_srv_event(srv, event)                                                                \
    do {                                                                                           \
        if((srv) != NULL) {                                                                        \
            if((srv)->driver != NULL) {                                                            \
                (srv)->driver(srv, event);                                                         \
            }                                                                                      \
        }                                                                                          \
    } while(0)

enum focus_event_type {
    FOCUS_EVENT_TYPE_CONTROL_INIT,
    FOCUS_EVENT_TYPE_CONTROL_CALIBRATE,
    FOCUS_EVENT_TYPE_CONTROL_START,
    FOCUS_EVENT_TYPE_CONTROL_LOOP,
    FOCUS_EVENT_TYPE_CONTROL_TASK,
    FOCUS_EVENT_TYPE_CONTROL_STOP,
    FOCUS_EVENT_TYPE_POSITION_SAMPLE,
    FOCUS_EVENT_TYPE_INVERTER_SAMPLE,
};

struct focus_event {
    enum focus_event_type type;
    union {
        struct {
            float rs;
            float ld;
            float lq;
        } control_start;
        struct {
            float i_ab[2];
            float i_dq[2];
            float u_dq[2];
            float u_ab[2];
        } control_loop;
        struct {
            float current_u;
            float current_v;
            float current_w;
            float voltage_vbus;
        } inverter_sample;
        struct {
            float position_electrical;
            float velocity_electrical;
            float position_mechanical;
            float velocity_mechanical;
        } position_sample;
    } arg;
};

struct focus_srv_control;
struct focus_srv_position;
struct focus_srv_inverter;
struct focus_port_position;
struct focus_port_inverter;

struct focus_srv_control {
    void (*driver)(struct focus_srv_control *, struct focus_event *);
    struct focus_srv_position *position;
    struct focus_srv_inverter *inverter;
};

struct focus_srv_position {
    void (*driver)(struct focus_srv_position *, struct focus_event *);
    struct focus_srv_control *control;
    struct focus_srv_inverter *inverter;
    struct focus_port_position *port;
};

struct focus_srv_inverter {
    void (*driver)(struct focus_srv_inverter *, struct focus_event *);
    struct focus_srv_control *control;
    struct focus_port_inverter *port;
};

struct focus_port_position {
    void (*driver)(struct focus_port_position *, struct focus_event *);
};

struct focus_port_inverter {
    void (*driver)(struct focus_port_inverter *, struct focus_event *);
};

#ifdef __cplusplus
}
#endif

#endif
