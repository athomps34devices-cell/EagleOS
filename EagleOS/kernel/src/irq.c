#include <stdint.h>
#include <irq.h>
#include <idt.h>
#include <pic.h>

static irq_handler_t irq_handlers[IRQ_COUNT];

extern void irq_stub_0(void);
extern void irq_stub_1(void);
extern void irq_stub_2(void);
extern void irq_stub_3(void);
extern void irq_stub_4(void);
extern void irq_stub_5(void);
extern void irq_stub_6(void);
extern void irq_stub_7(void);
extern void irq_stub_8(void);
extern void irq_stub_9(void);
extern void irq_stub_10(void);
extern void irq_stub_11(void);
extern void irq_stub_12(void);
extern void irq_stub_13(void);
extern void irq_stub_14(void);
extern void irq_stub_15(void);

static void (*const irq_stubs[IRQ_COUNT])(void) = {
    irq_stub_0,
    irq_stub_1,
    irq_stub_2,
    irq_stub_3,
    irq_stub_4,
    irq_stub_5,
    irq_stub_6,
    irq_stub_7,
    irq_stub_8,
    irq_stub_9,
    irq_stub_10,
    irq_stub_11,
    irq_stub_12,
    irq_stub_13,
    irq_stub_14,
    irq_stub_15
};

int irq_init(void) {
    __asm__ volatile ("cli");

    for (uint8_t irq = 0; irq < IRQ_COUNT; irq++) {
        irq_handlers[irq] = 0;

        idt_set_gate(
            IRQ_VECTOR_BASE + irq,
            irq_stubs[irq]
        );

        pic_set_mask(irq);
    }

    return 1;
}

int irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq >= IRQ_COUNT || handler == 0) {
        return 0;
    }

    __asm__ volatile ("cli");

    if (irq_handlers[irq] != 0) {
        return 0;
    }

    irq_handlers[irq] = handler;

    pic_clear_mask(irq);

    return 1;
}

int irq_unregister_handler(uint8_t irq) {
    if (irq >= IRQ_COUNT) {
        return 0;
    }

    __asm__ volatile ("cli");

    if (irq_handlers[irq] == 0) {
        return 0;
    }

    pic_set_mask(irq);

    irq_handlers[irq] = 0;

    return 1;
}

void irq_dispatch(uint64_t vector) {
    if (vector < IRQ_VECTOR_BASE ||
        vector >= IRQ_VECTOR_BASE + IRQ_COUNT) {
        return;
    }

    uint8_t irq = (uint8_t)(vector - IRQ_VECTOR_BASE);

    irq_handler_t handler = irq_handlers[irq];

    if (handler != 0) {
        handler();
    }

    pic_send_eoi(irq);
}