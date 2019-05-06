/* lib.h - Defines for useful library functions
 * vim:ts=4 noexpandtab
 */

#ifndef _LIB_H
#define _LIB_H

#define DIV_ZERO    0
#define SEGFAULT    1
#define INTERRUPT   2
#define ALARM       3
#define USER1       4

#define KEY_ARR_SIZE            128
#define CURSOR_MAX              15
#define MAX_TERMINAL_NUM        3
#define MAX_TASK                6               // maximum processes that can be run
#define FOUR_KB_SIZE            4096
#define EIGHT_KB_SIZE           8192
#define FOUR_MB_SIZE            0x00400000
#define EIGHT_MB_SIZE           0x00800000
#define _128_MB_SIZE            0x08000000
#define PCB_MASK                0x007FE000      // mask to get the current pcb pointer
#define TERMINAL_BUFFER_1       (0x000B8000 + FOUR_KB_SIZE)
#define TERMINAL_BUFFER_2       (0x000B8000 + 2 * FOUR_KB_SIZE)
#define TERMINAL_BUFFER_3       (0x000B8000 + 3 * FOUR_KB_SIZE)
#define CPU_CYCLE_PER_SEC       10000000

#include "types.h"

int32_t printf(int8_t *format, ...);
void putc(uint8_t c);
int32_t puts(int8_t *s);
int8_t *itoa(uint32_t value, int8_t* buf, int32_t radix);
int8_t *strrev(int8_t* s);
uint32_t strlen(const int8_t* s);
void clear(void);

int32_t print_width(int8_t * str, uint32_t width);

void* memset(void* s, int32_t c, uint32_t n);
void* memset_word(void* s, int32_t c, uint32_t n);
void* memset_dword(void* s, int32_t c, uint32_t n);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);
int8_t* strcpy(int8_t* dest, const int8_t*src);
int8_t* strncpy(int8_t* dest, const int8_t*src, uint32_t n);

/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);

// test interrrupts for rtc
void test_interrupts(void);
// turn the screen to blue when exception occurs
void blue_screen();

// Other helper functions add here!!!
void enable_cursor(uint8_t start, uint8_t end);
void backspace_handler();
void set_cursor_position(int x_, int y_);
void cursor_helper();
void enter_and_scroll();
void enter_handler();

void putc_terminal(uint8_t c, uint8_t terminal_id);
terminal_t terminal_info[MAX_TERMINAL_NUM];
void enter_and_scroll_terminal(uint8_t terminal_id);
void enter_handler_terminal(uint8_t terminal_id);

// Keyboard array
volatile uint8_t key_arr_all[MAX_TERMINAL_NUM][KEY_ARR_SIZE + 1];
volatile uint8_t* key_arr;
volatile uint8_t key_arr_ptr;
volatile uint8_t key_arr_overflow;
void key_arr_push(uint8_t elem);
void key_arr_pop();
void key_arr_clear();
void set_column_stack();
void copy_from_screen_buffer(uint8_t* des_addr);
void copy_to_screen_buffer(uint8_t* source);

int get_row_stack();
int get_column_stack();
int get_screen_x();
int get_screen_y();
int set_row_stack_in(int rs);
int set_column_stack_in(int cs);

// pcb helper functions
pcb_t* get_curr_pcb();
pcb_t* get_pcb_by_index(uint32_t index);
uint32_t process_terminal[MAX_TERMINAL_NUM][MAX_TASK];
uint32_t process_terminal_cnt[MAX_TERMINAL_NUM];
volatile uint8_t current_terminal;
volatile uint8_t running_terminal;


/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(port) {
    uint32_t val;
    asm volatile ("inl (%w1), %0"
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
    asm volatile ("outw %w1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
    asm volatile ("outl %l1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

#endif /* _LIB_H */
