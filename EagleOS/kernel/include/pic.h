#ifndef EAGLEOS_PIC_H
#define EAGLEOS_PIC_H

#include <stdint.h>

#define PIC_MASTER_OFFSET 0x20
#define PIC_SLAVE_OFFSET  0x28

#define PIC_IRQ_COUNT 16

void pic_init(void);

uint16_t pic_get_mask(void);

void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

void pic_send_eoi(uint8_t irq);

#endif