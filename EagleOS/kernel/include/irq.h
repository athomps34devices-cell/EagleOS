#ifndef EAGLEOS_IRQ_H
#define EAGLEOS_IRQ_H

#include <stdint.h>

#define IRQ_COUNT 16
#define IRQ_VECTOR_BASE 32

typedef void (*irq_handler_t)(void);

int irq_init(void);

int irq_register_handler(uint8_t irq, irq_handler_t handler);
int irq_unregister_handler(uint8_t irq);

void irq_dispatch(uint64_t vector);

#endif