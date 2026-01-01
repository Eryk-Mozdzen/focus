#ifndef FOCUS_H
#define FOCUS_H

#include <stdbool.h>
#include <stdint.h>

#ifndef FOCUS_LOG
#define FOCUS_LOG(format, ...) (void)0
#endif

/*typedef enum {
    FOCUS_STATUS_SUCCESS,
    FOCUS_STATUS_INVALID_ARG,
    FOCUS_STATUS_INVALID_DRIVER,
    FOCUS_STATUS_DRIVER_ERROR,
    FOCUS_STATUS_MOTOR_OVERCURRENT,
} focus_status_t;

typedef enum {
    FOCUS_STATE_ERROR,
    FOCUS_STATE_IDLE,
    FOCUS_STATE_CALIBRATION_CURRENT_LOOP,
    FOCUS_STATE_CALIBRATION_VELOCITY_LOOP,
    FOCUS_STATE_CALIBRATION_POSITION_LOOP,
    FOCUS_STATE_CONTROL_TORQUE,
    FOCUS_STATE_CONTROL_VELOCITY,
    FOCUS_STATE_CONTROL_POSITION,
} focus_state_t;

typedef struct {
    focus_state_t state;
    focus_status_t error;
    float voltage;
    float current_uvw[3];
    float current_dq[2];
    float current_setpoint;
    float position;
    float position_setpoint;
    float velocity;
    float velocity_setpoint;
} focus_telemetry_t;

typedef struct {
    void *user;
    uint32_t (*reset)(void *);
    uint32_t (*sample_start)(void *);
    uint32_t (*sample_get)(float *, void *);
} focus_encoder_t;

typedef struct {
    void *user;
    uint32_t (*reset)(void *);
    uint32_t (*set_gates)(const float, const float, const float, void *);
    uint32_t (*get_current)(float *, float *, float *, void *);
} focus_inverter_t;

typedef struct {
    void *user;
    uint32_t (*reset)(void *);
    uint32_t (*read)(const uint32_t, void *, const uint32_t, void *);
    uint32_t (*write)(const uint32_t, const void *, const uint32_t, void *);
    uint32_t (*flush)(void *);
} focus_memory_t;

focus_status_t focus_encoder_reset(focus_encoder_t *encoder);
focus_status_t focus_encoder_sample_start(focus_encoder_t *encoder);
focus_status_t focus_encoder_sample_get(focus_encoder_t *encoder, float *sample);

focus_status_t focus_inverter_reset(focus_inverter_t *inverter);
focus_status_t
focus_inverter_set_gates(focus_inverter_t *inverter, const float u, const float v, const float w);
focus_status_t focus_inverter_get_current(focus_inverter_t *inverter, float *u, float *v, float *w);

focus_status_t focus_memory_init(focus_memory_t memory);
focus_status_t
focus_memory_read(focus_memory_t memory, const uint32_t addr, void *data, const uint32_t len);
focus_status_t focus_memory_write(focus_memory_t *memory,
                                  const uint32_t addr,
                                  const void *data,
                                  const uint32_t len);
focus_status_t focus_memory_flush(focus_memory_t *memory);*/

typedef void (*focus_thread_callback_t)(void *);

typedef void (*focus_topic_subscriber_callback_t)(const char *,
                                                  const void *,
                                                  const uint32_t,
                                                  void *);

typedef struct focus_thread {
    struct focus_thread *next;
    focus_thread_callback_t callback;
    void *user;
} focus_thread_t;

typedef struct focus_topic {
    struct focus_topic *next;
    char *name;
    void *memory;
    uint32_t memory_size;
} focus_topic_t;

typedef struct focus_topic_subscriber {
    struct focus_topic_subscriber *next;
    focus_topic_subscriber_callback_t callback;
    void *user;
} focus_topic_subscriber_t;

void focus_thread_create(focus_thread_t *thread, const focus_thread_callback_t, void *user);
void focus_thread_destroy(const focus_thread_t *thread);

void focus_topic_create(focus_topic_t *topic,
                        const char *name,
                        void *memory,
                        const uint32_t memory_size);
void focus_subscriber_create(focus_topic_subscriber_t *subscriber,
                             const char *topic,
                             const focus_topic_subscriber_callback_t,
                             void *user);
uint32_t focus_publish(const char *topic, const void *data, const uint32_t data_size);

#endif
