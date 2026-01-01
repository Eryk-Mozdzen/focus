#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>

typedef struct {

} driver_inverter_t;

uint32_t driver_inverter_reset(void *user);
uint32_t driver_inverter_set_gates(const float u, const float v, const float w, void *user);
uint32_t driver_inverter_get_current(float *u, float *v, float *w, void *user);

typedef struct {

} driver_as5600_t;

uint32_t driver_as5600_reset(void *user);
uint32_t driver_as5600_sample_start(void *user);
uint32_t driver_as5600_sample_get(float *sample, void *user);

typedef struct {

} driver_flash_t;

uint32_t driver_flash_reset(void *user);
uint32_t driver_flash_read(const uint32_t addr, void *data, const uint32_t len, void *user);
uint32_t driver_flash_write(const uint32_t addr, const void *data, const uint32_t len, void *user);
uint32_t driver_flash_flush(void *user);

#endif
