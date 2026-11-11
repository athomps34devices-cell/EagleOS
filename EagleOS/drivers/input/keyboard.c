#include <stdint.h>
#include <keyboard.h>
#include <irq.h>

#define KEYBOARD_DATA_PORT       0x60
#define KEYBOARD_STATUS_PORT     0x64
#define KEYBOARD_COMMAND_PORT    0x64

#define KEYBOARD_BUFFER_SIZE     256

#define KEYBOARD_STATUS_OUTPUT   0x01
#define KEYBOARD_STATUS_INPUT    0x02
#define KEYBOARD_STATUS_TIMEOUT  0x40
#define KEYBOARD_STATUS_PARITY   0x80

#define KEYBOARD_ACK             0xFA
#define KEYBOARD_RESEND          0xFE

#define KEYBOARD_LEFT_SHIFT      0x2A
#define KEYBOARD_RIGHT_SHIFT     0x36
#define KEYBOARD_CAPS_LOCK       0x3A

static volatile uint8_t scancode_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint16_t buffer_head = 0;
static volatile uint16_t buffer_tail = 0;

static volatile int initialized = 0;

static int left_shift_pressed = 0;
static int right_shift_pressed = 0;
static int caps_lock_enabled = 0;

static const char normal_keymap[128] = {
    [0x01] = 27,
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '-',
    [0x0D] = '=',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x1A] = '[',
    [0x1B] = ']',

    [0x1C] = '\n',
    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',
    [0x28] = '\'',
    [0x29] = '`',

    [0x2B] = '\\',
    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x37] = '*',
    [0x39] = ' '
};

static const char shifted_keymap[128] = {
    [0x01] = 27,
    [0x02] = '!',
    [0x03] = '@',
    [0x04] = '#',
    [0x05] = '$',
    [0x06] = '%',
    [0x07] = '^',
    [0x08] = '&',
    [0x09] = '*',
    [0x0A] = '(',
    [0x0B] = ')',
    [0x0C] = '_',
    [0x0D] = '+',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'Q',
    [0x11] = 'W',
    [0x12] = 'E',
    [0x13] = 'R',
    [0x14] = 'T',
    [0x15] = 'Y',
    [0x16] = 'U',
    [0x17] = 'I',
    [0x18] = 'O',
    [0x19] = 'P',
    [0x1A] = '{',
    [0x1B] = '}',

    [0x1C] = '\n',
    [0x1E] = 'A',
    [0x1F] = 'S',
    [0x20] = 'D',
    [0x21] = 'F',
    [0x22] = 'G',
    [0x23] = 'H',
    [0x24] = 'J',
    [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = ':',
    [0x28] = '"',
    [0x29] = '~',

    [0x2B] = '|',
    [0x2C] = 'Z',
    [0x2D] = 'X',
    [0x2E] = 'C',
    [0x2F] = 'V',
    [0x30] = 'B',
    [0x31] = 'N',
    [0x32] = 'M',
    [0x33] = '<',
    [0x34] = '>',
    [0x35] = '?',
    [0x37] = '*',
    [0x39] = ' '
};

static inline uint8_t inb(uint16_t port) {
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void io_wait(void) {
    outb(0x80, 0);
}

static int wait_input_clear(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_INPUT)) {
            return 1;
        }
    }

    return 0;
}

static int wait_output_full(void) {
    for (uint32_t i = 0; i < 100000; i++) {
        if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT) {
            return 1;
        }
    }

    return 0;
}

static int controller_command(uint8_t command) {
    if (!wait_input_clear()) {
        return 0;
    }

    outb(KEYBOARD_COMMAND_PORT, command);
    io_wait();

    return 1;
}

static int controller_write_config(uint8_t config) {
    if (!wait_input_clear()) {
        return 0;
    }

    outb(KEYBOARD_COMMAND_PORT, 0x60);

    if (!wait_input_clear()) {
        return 0;
    }

    outb(KEYBOARD_DATA_PORT, config);
    io_wait();

    return 1;
}

static int controller_read_config(uint8_t *config) {
    if (!config) {
        return 0;
    }

    if (!controller_command(0x20)) {
        return 0;
    }

    if (!wait_output_full()) {
        return 0;
    }

    *config = inb(KEYBOARD_DATA_PORT);

    return 1;
}

static int keyboard_send_command(uint8_t command) {
    for (uint32_t attempt = 0; attempt < 3; attempt++) {
        if (!wait_input_clear()) {
            return 0;
        }

        outb(KEYBOARD_DATA_PORT, command);

        if (!wait_output_full()) {
            return 0;
        }

        uint8_t response = inb(KEYBOARD_DATA_PORT);

        if (response == KEYBOARD_ACK) {
            return 1;
        }

        if (response != KEYBOARD_RESEND) {
            return 0;
        }
    }

    return 0;
}

static void keyboard_interrupt_handler(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    if (!(status & KEYBOARD_STATUS_OUTPUT)) {
        return;
    }

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (status & (KEYBOARD_STATUS_TIMEOUT | KEYBOARD_STATUS_PARITY)) {
        return;
    }

    uint16_t next = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;

    if (next == buffer_tail) {
        return;
    }

    scancode_buffer[buffer_head] = scancode;

    __asm__ volatile ("" ::: "memory");

    buffer_head = next;
}

static int keyboard_controller_init(void) {
    if (!controller_command(0xAD)) {
        return 0;
    }

    if (!controller_command(0xA7)) {
        return 0;
    }

    for (uint32_t i = 0; i < 10000; i++) {
        if (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT)) {
            break;
        }

        (void)inb(KEYBOARD_DATA_PORT);
    }

    uint8_t config;

    if (!controller_read_config(&config)) {
        return 0;
    }

    config |= (1 << 0);
    config |= (1 << 6);
    config |= (1 << 5);

    config &= ~(1 << 4);

    if (!controller_write_config(config)) {
        return 0;
    }

    if (!controller_command(0xAE)) {
        return 0;
    }

    return 1;
}

int keyboard_init(void) {
    __asm__ volatile ("cli" ::: "memory");

    initialized = 0;

    buffer_head = 0;
    buffer_tail = 0;

    left_shift_pressed = 0;
    right_shift_pressed = 0;
    caps_lock_enabled = 0;

    if (!keyboard_controller_init()) {
        return 0;
    }

    if (!keyboard_send_command(0xF5)) {
        return 0;
    }

    if (!keyboard_send_command(0xF4)) {
        return 0;
    }

    if (!irq_register_handler(1, keyboard_interrupt_handler)) {
        return 0;
    }

    initialized = 1;

    return 1;
}

char keyboard_read_char(void) {
    if (!initialized) {
        return 0;
    }

    while (buffer_tail != buffer_head) {
        uint8_t scancode = scancode_buffer[buffer_tail];

        __asm__ volatile ("" ::: "memory");

        buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;

        if (scancode == 0xE0 || scancode == 0xE1) {
            continue;
        }

        int released = (scancode & 0x80) != 0;
        uint8_t code = scancode & 0x7F;

        if (code == KEYBOARD_LEFT_SHIFT) {
            left_shift_pressed = !released;
            continue;
        }

        if (code == KEYBOARD_RIGHT_SHIFT) {
            right_shift_pressed = !released;
            continue;
        }

        if (code == KEYBOARD_CAPS_LOCK) {
            if (!released) {
                caps_lock_enabled = !caps_lock_enabled;
            }

            continue;
        }

        if (released) {
            continue;
        }

        int shift_pressed =
            left_shift_pressed || right_shift_pressed;

        char normal = normal_keymap[code];
        char shifted = shifted_keymap[code];

        if (normal >= 'a' && normal <= 'z') {
            if (shift_pressed ^ caps_lock_enabled) {
                return shifted;
            }

            return normal;
        }

        if (shift_pressed && shifted != 0) {
            return shifted;
        }

        return normal;
    }

    return 0;
}

int keyboard_is_initialized(void) {
    return initialized;
}