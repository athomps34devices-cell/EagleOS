#ifndef EAGLEOS_PIT_H
#define EAGLEOS_PIT_H

#include <stdint.h>

#define PIT_INPUT_FREQUENCY 1193182
#define PIT_DEFAULT_FREQUENCY 100

int pit_init(uint32_t frequency);

uint64_t pit_get_ticks(void);
uint32_t pit_get_frequency(void);

#endif