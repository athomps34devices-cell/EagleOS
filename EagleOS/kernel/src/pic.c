#include <stdint.h>
#include <pic.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA    0x21

#define PIC_SLAVE_COMMAND  0xA0
#define PIC_SLAVE_DATA     0xA1

#define PIC_EOI 0x20

#define PIC_ICW1_INIT  0x10
#define PIC_ICW1_ICW4  0x01

#define PIC_ICW4_8086 0x01

#define PIC_CASCADE_IRQ 2

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_init(void) {
    __asm__ volatile ("cli");

    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA, 0xFF);

    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET);
    io_wait();

    outb(PIC_SLAVE_DATA, PIC_SLAVE_OFFSET);
    io_wait();

    outb(PIC_MASTER_DATA, 1 << PIC_CASCADE_IRQ);
    io_wait();

    outb(PIC_SLAVE_DATA, 2);
    io_wait();

    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    io_wait();

    outb(PIC_SLAVE_DATA, PIC_ICW4_8086);
    io_wait();

    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA, 0xFF);
}

uint16_t pic_get_mask(void) {
    uint8_t master_mask = inb(PIC_MASTER_DATA);
    uint8_t slave_mask = inb(PIC_SLAVE_DATA);

    return (uint16_t)master_mask |
           ((uint16_t)slave_mask << 8);
}

void pic_set_mask(uint8_t irq) {
    if (irq >= PIC_IRQ_COUNT) {
        return;
    }

    uint16_t port;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    uint8_t mask = inb(port);
    mask |= (uint8_t)(1U << irq);

    outb(port, mask);
}

void pic_clear_mask(uint8_t irq) {
    if (irq >= PIC_IRQ_COUNT) {
        return;
    }

    uint16_t port;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;

        uint8_t master_mask = inb(PIC_MASTER_DATA);

        master_mask &= (uint8_t)~(1U << PIC_CASCADE_IRQ);

        outb(PIC_MASTER_DATA, master_mask);
    }

    uint8_t mask = inb(port);

    mask &= (uint8_t)~(1U << irq);

    outb(port, mask);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= PIC_IRQ_COUNT) {
        return;
    }

    if (irq >= 8) {
        outb(PIC_SLAVE_COMMAND, PIC_EOI);
    }

    outb(PIC_MASTER_COMMAND, PIC_EOI);
}