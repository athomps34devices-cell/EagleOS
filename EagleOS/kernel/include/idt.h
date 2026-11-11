#ifndef EAGLEOS_IDT_H
#define EAGLEOS_IDT_H

#include <stdint.h>

int idt_init(void);

void idt_set_gate(uint8_t vector, void (*handler)(void));

#endif