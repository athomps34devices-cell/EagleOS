#include <stdint.h>
#include <pit.h>
#include <irq.h>

#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND   0x43

#define PIT_COMMAND_CHANNEL_0 0x00
#define PIT_ACCESS_LOHI       0x30
#define PIT_MODE_SQUARE_WAVE  0x06
#define PIT_BINARY_MODE       0x00

static volatile uint64_t pit_ticks = 0;
static uint32_t pit_frequency = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void pit_irq_handler(void) {
    pit_ticks++;
}

int pit_init(uint32_t frequency) {
    if (frequency == 0) {
        return 0;
    }

    uint32_t divisor = PIT_INPUT_FREQUENCY / frequency;

    if (divisor < 1) {
        divisor = 1;
    }

    if (divisor > 65535) {
        divisor = 65535;
    }

    pit_frequency = PIT_INPUT_FREQUENCY / divisor;

    uint8_t command =
        PIT_COMMAND_CHANNEL_0 |
        PIT_ACCESS_LOHI |
        PIT_MODE_SQUARE_WAVE |
        PIT_BINARY_MODE;

    outb(PIT_COMMAND, command);

    outb(PIT_CHANNEL_0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_0, (uint8_t)((divisor >> 8) & 0xFF));

    if (!irq_register_handler(0, pit_irq_handler)) {
        return 0;
    }

    return 1;
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

uint32_t pit_get_frequency(void) {
    return pit_frequency;
}