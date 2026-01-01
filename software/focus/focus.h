#ifndef FOCUS_H
#define FOCUS_H

#include <stdint.h>

typedef void (*focus_subscriber_callback_t)(const char *, const void *, const uint32_t, void *);

typedef struct {
    char *name;
    void *memory;
    uint32_t memory_size;
} focus_topic_t;

typedef struct {
    void *user;
    char *topic;
    focus_subscriber_callback_t callback;
} focus_subscriber_t;

void focus_topic_create(const char *name, void *memory, const uint32_t memory_size);
void focus_subscriber_create(const char *topic, const pb_subscriber_callback_t, void *user);
uint32_t focus_publish(const char *topic, const void *data, const uint32_t data_size);

#endif
