#ifndef EAGLEOS_KEYBOARD_H
#define EAGLEOS_KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256

int keyboard_init(void);

char keyboard_read_char(void);

int keyboard_is_initialized(void);

#endif