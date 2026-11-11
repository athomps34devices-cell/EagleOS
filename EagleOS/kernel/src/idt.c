#include <stdint.h>
#include <stddef.h>
#include <idt.h>

#define IDT_ENTRIES 256

#define IDT_INTERRUPT_GATE 0x8E

#define SERIAL_PORT 0x3F8

struct interrupt_frame {
    uint64_t instruction_pointer;
    uint64_t code_segment;
    uint64_t cpu_flags;
    uint64_t stack_pointer;
    uint64_t stack_segment;
};

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES]
    __attribute__((aligned(16)));

static struct idt_pointer idtr;

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

static void serial_write_char(char character) {
    uint32_t timeout = 1000000;

    while (!(inb(SERIAL_PORT + 5) & 0x20)) {
        if (--timeout == 0) {
            return;
        }
    }

    outb(SERIAL_PORT, (uint8_t)character);
}

static void serial_write(const char *text) {
    while (*text) {
        if (*text == '\n') {
            serial_write_char('\r');
        }

        serial_write_char(*text++);
    }
}

static void serial_write_hex(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";

    serial_write("0x");

    int started = 0;

    for (int shift = 60; shift >= 0; shift -= 4) {
        uint8_t digit = (value >> shift) & 0xF;

        if (digit || started || shift == 0) {
            serial_write_char(digits[digit]);
            started = 1;
        }
    }
}

static void serial_write_decimal(uint64_t value) {
    char buffer[21];
    uint32_t index = 0;

    if (value == 0) {
        serial_write_char('0');
        return;
    }

    while (value > 0) {
        buffer[index++] = '0' + (value % 10);
        value /= 10;
    }

    while (index > 0) {
        serial_write_char(buffer[--index]);
    }
}

static const char *exception_name(uint64_t vector) {
    switch (vector) {
        case 0: return "Division Error";
        case 1: return "Debug";
        case 2: return "Non-Maskable Interrupt";
        case 3: return "Breakpoint";
        case 4: return "Overflow";
        case 5: return "Bound Range Exceeded";
        case 6: return "Invalid Opcode";
        case 7: return "Device Not Available";
        case 8: return "Double Fault";
        case 9: return "Coprocessor Segment Overrun";
        case 10: return "Invalid TSS";
        case 11: return "Segment Not Present";
        case 12: return "Stack-Segment Fault";
        case 13: return "General Protection Fault";
        case 14: return "Page Fault";
        case 16: return "x87 Floating-Point Exception";
        case 17: return "Alignment Check";
        case 18: return "Machine Check";
        case 19: return "SIMD Floating-Point Exception";
        case 20: return "Virtualization Exception";
        case 21: return "Control Protection Exception";
        case 28: return "Hypervisor Injection Exception";
        case 29: return "VMM Communication Exception";
        case 30: return "Security Exception";
        default: return "Unknown CPU Exception";
    }
}

static __attribute__((noreturn))
void exception_halt(void) {
    for (;;) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

static __attribute__((noreturn))
void exception_fatal(
    uint64_t vector,
    uint64_t error_code,
    uint64_t instruction_pointer
) {
    serial_write("\n");
    serial_write("========================================\n");
    serial_write("       EAGLEOS CPU EXCEPTION\n");
    serial_write("========================================\n");

    serial_write("[EagleOS] Exception: ");
    serial_write(exception_name(vector));
    serial_write("\n");

    serial_write("[EagleOS] Vector: ");
    serial_write_decimal(vector);
    serial_write("\n");

    serial_write("[EagleOS] Error code: ");
    serial_write_hex(error_code);
    serial_write("\n");

    serial_write("[EagleOS] Instruction pointer: ");
    serial_write_hex(instruction_pointer);
    serial_write("\n");

    if (vector == 14) {
        uint64_t fault_address;

        __asm__ volatile (
            "mov %%cr2, %0"
            : "=r"(fault_address)
        );

        serial_write("[EagleOS] Faulting virtual address: ");
        serial_write_hex(fault_address);
        serial_write("\n");

        serial_write("[EagleOS] Page fault reason: ");

        if (error_code & 1) {
            serial_write("Protection violation");
        } else {
            serial_write("Page not present");
        }

        serial_write("\n");

        serial_write("[EagleOS] Access type: ");

        if (error_code & 2) {
            serial_write("Write");
        } else {
            serial_write("Read");
        }

        serial_write("\n");

        serial_write("[EagleOS] Privilege: ");

        if (error_code & 4) {
            serial_write("User mode");
        } else {
            serial_write("Supervisor mode");
        }

        serial_write("\n");

        if (error_code & 8) {
            serial_write("[EagleOS] Reserved page-table bit violation.\n");
        }

        if (error_code & 16) {
            serial_write("[EagleOS] Instruction fetch fault.\n");
        }

        if (error_code & 32) {
            serial_write("[EagleOS] Protection key violation.\n");
        }

        if (error_code & 64) {
            serial_write("[EagleOS] Shadow stack access fault.\n");
        }

        if (error_code & 32768) {
            serial_write("[EagleOS] SGX-related fault.\n");
        }
    }

    serial_write("[EagleOS] Kernel halted due to CPU exception.\n");

    exception_halt();
}

#define DEFINE_EXCEPTION_NO_ERROR(number) \
    static __attribute__((interrupt)) \
    void exception_##number(struct interrupt_frame *frame) { \
        exception_fatal(number, 0, frame->instruction_pointer); \
    }

#define DEFINE_EXCEPTION_ERROR(number) \
    static __attribute__((interrupt)) \
    void exception_##number( \
        struct interrupt_frame *frame, \
        uint64_t error_code \
    ) { \
        exception_fatal(number, error_code, frame->instruction_pointer); \
    }

DEFINE_EXCEPTION_NO_ERROR(0)
DEFINE_EXCEPTION_NO_ERROR(1)
DEFINE_EXCEPTION_NO_ERROR(2)
DEFINE_EXCEPTION_NO_ERROR(3)
DEFINE_EXCEPTION_NO_ERROR(4)
DEFINE_EXCEPTION_NO_ERROR(5)
DEFINE_EXCEPTION_NO_ERROR(6)
DEFINE_EXCEPTION_NO_ERROR(7)

DEFINE_EXCEPTION_ERROR(8)
DEFINE_EXCEPTION_NO_ERROR(9)
DEFINE_EXCEPTION_ERROR(10)
DEFINE_EXCEPTION_ERROR(11)
DEFINE_EXCEPTION_ERROR(12)
DEFINE_EXCEPTION_ERROR(13)
DEFINE_EXCEPTION_ERROR(14)
DEFINE_EXCEPTION_NO_ERROR(15)

DEFINE_EXCEPTION_NO_ERROR(16)
DEFINE_EXCEPTION_ERROR(17)
DEFINE_EXCEPTION_NO_ERROR(18)
DEFINE_EXCEPTION_NO_ERROR(19)
DEFINE_EXCEPTION_NO_ERROR(20)
DEFINE_EXCEPTION_ERROR(21)
DEFINE_EXCEPTION_NO_ERROR(22)
DEFINE_EXCEPTION_NO_ERROR(23)

DEFINE_EXCEPTION_NO_ERROR(24)
DEFINE_EXCEPTION_NO_ERROR(25)
DEFINE_EXCEPTION_NO_ERROR(26)
DEFINE_EXCEPTION_NO_ERROR(27)
DEFINE_EXCEPTION_NO_ERROR(28)
DEFINE_EXCEPTION_ERROR(29)
DEFINE_EXCEPTION_ERROR(30)
DEFINE_EXCEPTION_NO_ERROR(31)

void idt_set_gate(
    uint8_t vector,
    void (*handler)(void)
) {
    uint64_t address = (uint64_t)handler;

    uint16_t code_selector;

    __asm__ volatile (
        "mov %%cs, %0"
        : "=r"(code_selector)
    );

    idt[vector].offset_low = address & 0xFFFF;
    idt[vector].selector = code_selector;
    idt[vector].ist = 0;
    idt[vector].attributes = IDT_INTERRUPT_GATE;
    idt[vector].offset_middle = (address >> 16) & 0xFFFF;
    idt[vector].offset_high = (address >> 32) & 0xFFFFFFFF;
    idt[vector].reserved = 0;
}

int idt_init(void) {
    __asm__ volatile ("cli");

    for (uint64_t i = 0; i < IDT_ENTRIES; i++) {
        idt[i] = (struct idt_entry){0};
    }

    void (*handlers[32])(void) = {
        [0] = (void (*)(void))exception_0,
        [1] = (void (*)(void))exception_1,
        [2] = (void (*)(void))exception_2,
        [3] = (void (*)(void))exception_3,
        [4] = (void (*)(void))exception_4,
        [5] = (void (*)(void))exception_5,
        [6] = (void (*)(void))exception_6,
        [7] = (void (*)(void))exception_7,
        [8] = (void (*)(void))exception_8,
        [9] = (void (*)(void))exception_9,
        [10] = (void (*)(void))exception_10,
        [11] = (void (*)(void))exception_11,
        [12] = (void (*)(void))exception_12,
        [13] = (void (*)(void))exception_13,
        [14] = (void (*)(void))exception_14,
        [15] = (void (*)(void))exception_15,
        [16] = (void (*)(void))exception_16,
        [17] = (void (*)(void))exception_17,
        [18] = (void (*)(void))exception_18,
        [19] = (void (*)(void))exception_19,
        [20] = (void (*)(void))exception_20,
        [21] = (void (*)(void))exception_21,
        [22] = (void (*)(void))exception_22,
        [23] = (void (*)(void))exception_23,
        [24] = (void (*)(void))exception_24,
        [25] = (void (*)(void))exception_25,
        [26] = (void (*)(void))exception_26,
        [27] = (void (*)(void))exception_27,
        [28] = (void (*)(void))exception_28,
        [29] = (void (*)(void))exception_29,
        [30] = (void (*)(void))exception_30,
        [31] = (void (*)(void))exception_31
    };

    for (uint8_t i = 0; i < 32; i++) {
        idt_set_gate(i, handlers[i]);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(idtr)
    );

    return 1;
}