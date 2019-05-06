/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"

#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    24
#define ATTRIB_0    ((0xC << 4) | 0xE)
#define ATTRIB_1    ((0x8 << 4) | 0xA)
#define ATTRIB_2    ((0x9 << 4) | 0xA)
#define ATTRIB_BAR  ((0xF << 4) | 0x0)
#define ATTRIB_HIG  ((0xF << 4) | 0xD)

#define HIGH_1      13
#define HIGH_2      35
#define HIGH_3      57
#define BARTEXLEN   10

#define BLUE        0x17

uint8_t attrib[MAX_TERMINAL_NUM] = {ATTRIB_0, ATTRIB_1, ATTRIB_2};
uint8_t high[MAX_TERMINAL_NUM] = {HIGH_1, HIGH_2, HIGH_3};
uint32_t saved_screen[MAX_TERMINAL_NUM] = {TERMINAL_BUFFER_1, TERMINAL_BUFFER_2, TERMINAL_BUFFER_3};

int8_t* bartext = "             TERMINAL 1            TERMINAL 2            TERMINAL 3             ";

static int screen_x;
static int screen_y;
static int row_stack = 0;
static int column_stack = 0;
static char* video_mem = (char *)VIDEO;

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = attrib[current_terminal];
    }
    for (i = NUM_ROWS * NUM_COLS; i < (NUM_ROWS + 1) * NUM_COLS; i ++) {
        *(uint8_t *)(video_mem + (i << 1)) = bartext[i - NUM_ROWS * NUM_COLS];
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB_BAR;
    }
    for (i = NUM_ROWS * NUM_COLS + high[current_terminal]; i <  NUM_ROWS * NUM_COLS + high[current_terminal] + BARTEXLEN; i++){
        *(uint8_t *)(video_mem + (i << 1)) = bartext[i - NUM_ROWS * NUM_COLS];
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB_HIG;
    }

}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(uint8_t c) {
    int i;
    if(c == '\n' || c == '\r') {
        enter_handler();
        cursor_helper();
    } else {
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib[current_terminal];
        screen_x++;
        if(screen_x >= NUM_COLS){

            row_stack ++;
            if(row_stack > NUM_ROWS - 1){
                row_stack = NUM_ROWS - 1;
            }
            screen_x %= NUM_COLS;

            if(screen_y < NUM_ROWS - 1){

                screen_y ++;

            }else{

                memcpy((void*)(video_mem),
                        (void*)(video_mem + (NUM_COLS << 1)), ((NUM_ROWS - 1) * NUM_COLS) << 1); // 3840 is the size of video_mem row1 -- row 24
                screen_y = NUM_ROWS - 1;
                screen_x = 0;
                for(i = 0; i < NUM_COLS; i ++){
                    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + i) << 1)) = ' ';
                    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + i) << 1) + 1) = attrib[current_terminal];
                }
            }

        }
        screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
        cursor_helper();
    }
}

/* void putc_terminal(uint8_t c, uint8_t terminal_id);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 * Function:    print a char to the corresponding terminal buffer
 *              when the running terminal is not the current terminal 
 */
void putc_terminal(uint8_t c, uint8_t terminal_id){
    int i;
    if(c == '\n' || c == '\r') {
        enter_handler_terminal(terminal_id);
        //cursor_helper();
    } else {
        //printf("%d %d ,", terminal_info[terminal_id].screen_pos_x,  terminal_info[terminal_id].screen_pos_y);
        *(uint8_t *)(saved_screen[terminal_id] + ((NUM_COLS * terminal_info[terminal_id].screen_pos_y + terminal_info[terminal_id].screen_pos_x) << 1)) = c;
        *(uint8_t *)(saved_screen[terminal_id] + ((NUM_COLS * terminal_info[terminal_id].screen_pos_y + terminal_info[terminal_id].screen_pos_x) << 1) + 1) = attrib[terminal_id];
        terminal_info[terminal_id].screen_pos_x++;
        if(terminal_info[terminal_id].screen_pos_x >= NUM_COLS){

            terminal_info[terminal_id].row_stack_old ++;
            if(terminal_info[terminal_id].row_stack_old > NUM_ROWS - 1){
                terminal_info[terminal_id].row_stack_old = NUM_ROWS - 1;
            }
            terminal_info[terminal_id].screen_pos_x %= NUM_COLS;

            if(terminal_info[terminal_id].screen_pos_y < NUM_ROWS - 1){

                terminal_info[terminal_id].screen_pos_y ++;

            }else{

                memcpy((void*)(saved_screen[terminal_id]),
                        (void*)(saved_screen[terminal_id] + (NUM_COLS << 1)), ((NUM_ROWS - 1) * NUM_COLS) << 1); // 3840 is the size of video_mem row1 -- row 24
                terminal_info[terminal_id].screen_pos_y = NUM_ROWS - 1;
                terminal_info[terminal_id].screen_pos_x = 0;
                for(i = 0; i < NUM_COLS; i ++){
                    *(uint8_t *)(saved_screen[terminal_id] + ((NUM_COLS * terminal_info[terminal_id].screen_pos_y + i) << 1)) = ' ';
                    *(uint8_t *)(saved_screen[terminal_id] + ((NUM_COLS * terminal_info[terminal_id].screen_pos_y + i) << 1) + 1) = attrib[terminal_id];
                }
            }

        }
        terminal_info[terminal_id].screen_pos_y = (terminal_info[terminal_id].screen_pos_y + (terminal_info[terminal_id].screen_pos_x / NUM_COLS)) % NUM_ROWS;
        //cursor_helper();
    }
}

/* void enter_handler();
 * Description: handler for enter
 * Inputs: None
 * Return Value: void
 */
void enter_handler(){
    uint8_t adjust = 0;
    if(screen_x == 0 && row_stack != 0){
        adjust = 1;
    }
    if(screen_y >= NUM_ROWS - 1){
        enter_and_scroll();
    }else{
        set_cursor_position(0, screen_y + 1);
    }
    if(adjust){
        screen_y --;
        cursor_helper();
    }
    row_stack = 0;
}


/* void enter_and_scroll();
 * Description: handle the case that need to scroll when press enter
 * Inputs: None
 * Return Value: void
 */

void enter_and_scroll(){
    int i;
    memcpy((void*)(video_mem),
           (void*)(video_mem + (NUM_COLS << 1)), 3840); // 3840 is the size of video_mem row1 -- row 24

    screen_y = NUM_ROWS - 1;
    screen_x = 0;

    for(i = 0; i < NUM_COLS; i ++){
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + i) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + i) << 1) + 1) = attrib[current_terminal];
    }

    screen_y = NUM_ROWS - 1;
    screen_x = 0;
    cursor_helper();
    return;
}

void enter_and_scroll_terminal(uint8_t terminal_id){
    int i;
    memcpy((void*)(saved_screen[terminal_id]),
           (void*)(saved_screen[terminal_id] + (NUM_COLS << 1)), 3840); // 3840 is the size of video_mem row1 -- row 24

    terminal_info[terminal_id].screen_pos_y = NUM_ROWS - 1;
    terminal_info[terminal_id].screen_pos_x = 0;

    for(i = 0; i < NUM_COLS; i ++){
        *(uint8_t *)(saved_screen[terminal_id] + ((NUM_COLS * terminal_info[terminal_id].screen_pos_y + i) << 1)) = ' ';
        *(uint8_t *)(saved_screen[terminal_id] + ((NUM_COLS * terminal_info[terminal_id].screen_pos_y + i) << 1) + 1) = attrib[terminal_id];
    }

    terminal_info[terminal_id].screen_pos_y = NUM_ROWS - 1;
    terminal_info[terminal_id].screen_pos_x = 0;
    //cursor_helper();
    return;
}
void enter_handler_terminal(uint8_t terminal_id){
    uint8_t adjust = 0;
    if(terminal_info[terminal_id].screen_pos_x == 0 && terminal_info[terminal_id].row_stack_old != 0){
        adjust = 1;
    }
    if(terminal_info[terminal_id].screen_pos_y >= NUM_ROWS - 1){
        enter_and_scroll_terminal(terminal_id);
    }else{
        //set_cursor_position(0, screen_y + 1);
        terminal_info[terminal_id].screen_pos_x = 0;
        if(terminal_info[terminal_id].screen_pos_y >=  NUM_ROWS){
            terminal_info[terminal_id].screen_pos_y = NUM_ROWS;
        }else{
            terminal_info[terminal_id].screen_pos_y += 1;
        }
        terminal_info[terminal_id].row_stack_old = 0;
    }
    if(adjust){
        terminal_info[terminal_id].screen_pos_y --;
        //cursor_helper();
    }
    terminal_info[terminal_id].row_stack_old = 0;
}


/* void backspace_handler();
 * Description: handle backspace from keyboard
 * Inputs: None
 * Return Value: void
 */

void backspace_handler(){
    if(screen_x == column_stack && row_stack == 0){
        return;
    }else if(screen_x == 0 && row_stack != 0){
        screen_x = NUM_COLS - 1;
        screen_y --;
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib[current_terminal];
        row_stack --;
        key_arr_pop();
    }else{
        screen_x --;
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = attrib[current_terminal];
        key_arr_pop();
    }
    cursor_helper();
    return;
}


/* void set_cursor_position(int x_, int y_);
 * Description: set the cursor to desired position
 * Inputs: new coordinates for screen_x and screen_y
 * Return Value: void
 */

void set_cursor_position(int x_, int y_){
    screen_x = x_;
    if(y_ >= NUM_ROWS){
        y_ = NUM_ROWS;
    }
    screen_y = y_;

    cursor_helper();
    row_stack = 0;
    return;
}

/* void cursor_helper();
 * Description: set the position of the cursor to screen_x and screen_y
 * Inputs: None
 * Return Value: void
 * Reference: https://wiki.osdev.org/Text_Mode_Cursor
 */
void cursor_helper(){
    uint16_t pos = screen_y * NUM_COLS + screen_x;

	outb(0x0F, 0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);

    return;
}

/* void enable_cursor(uint8_t start, uint8_t end);
 * Description: display the cursor onto the screen
 * Inputs: uint8_t start = start value
 *           uint8_t end = end value
 * Return Value: void
 * Reference: https://wiki.osdev.org/Text_Mode_Cursor
 */
void enable_cursor(uint8_t start, uint8_t end)
{
	outb(0x0A, 0x3D4);
	outb((inb(0x3D5) & 0xC0) | start, 0x3D5);

	outb(0x0B, 0x3D4);
	outb((inb(0x3D5) & 0xE0) | end, 0x3D5);
}


/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
        // video_mem[i%NUM_COLS]++;
    }
}

/* void blue_screen(void)
 * Inputs: void
 * Return Value: void
 * Function: turn the screen to blue */
void blue_screen() {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1) + 1) = BLUE;
    }
}

/* print_width(int8_t * str, uint32_t width)
 * Inputs:  str: pointer to a string
 *          width: desired width
 * Return Value: 0 if succeed, n if n characters is not printed
 * Function: turn the screen to blue */
int32_t print_width(int8_t * str, uint32_t width){
    uint32_t i;
    int8_t str_to_print[width + 1];
    uint32_t len = strlen(str);
    uint32_t num_space;

    str_to_print[width] = '\0';

    if(len > width){

        strncpy((int8_t*)&(str_to_print[0]), (int8_t*)str, width);
        puts(str_to_print);
        return len - width;

    }

    num_space = width - len;

    for(i = 0; i < num_space; i ++){
        str_to_print[i] = ' ';
    }

    strncpy((int8_t*)&(str_to_print[num_space]), (int8_t*)str, len);
    puts(str_to_print);

    return 0;
}


/* void key_arr_push(uint8_t elem)
 * Inputs: elem: the elem to be pushed into array
 * Return Value: none
 * Function: pushed the given element into array
 *
 */

void key_arr_push(uint8_t elem){
    if(key_arr_ptr < (KEY_ARR_SIZE - 1)){
        key_arr[key_arr_ptr ++] = elem;
    }else{
        key_arr_overflow ++;
    }
    return;
}


/* void key_arr_push(uint8_t elem)
 * Inputs: elem: the elem to be pushed into array
 * Return Value: none
 * Function: When backspace is pressed, check if the array is already overflow
 *           pop the last element when not.
 */

 void key_arr_pop(){
     if(key_arr_overflow){
         key_arr_overflow --;
     }else{
         if(key_arr_ptr){
             key_arr_ptr --;
         }else{
             return;
         }
     }
     return;
 }

 /* void key_arr_clear()
 * Inputs: none
 * Return Value: none
 * Function: Set the pointer to head of the array, set overflow value to 0
 *
 */

void key_arr_clear(){
    int32_t i = 0;
    key_arr_ptr = 0;
    key_arr_overflow = 0;
    for(i = 0; i < KEY_ARR_SIZE + 1; i ++){
        key_arr[i] = '\0';
    }
    return;
}

/*  set_column_stack()
*-----------------------------------------------------------------------------------
*   Description: Helper function that Set the most left character that backspace can delete to screen_x
*   Input: None
*   Return: None
*   Side Effect: None
*/
void set_column_stack(){
    column_stack = screen_x;
    return;
}


/*  void copy_from_screen_buffer(uint8_t* des_addr)
*-----------------------------------------------------------------------------------
*   Description: Helper function that save the screen buffer to an assigned address
*   Input: the address that we want to save the current videomem buffer
*   Return: None
*   Side Effect: change the content that des_addr points to (25 * 80 * 2)
*/
void copy_from_screen_buffer(uint8_t* des_addr){
    memcpy(des_addr, (uint8_t*)video_mem, (NUM_COLS * NUM_ROWS) << 1);
}

/*  void copy_to_screen_buffer(uint8_t* des_addr)
*-----------------------------------------------------------------------------------
*   Description: Helper function that save the screen buffer to an assigned address
*   Input: the address that we want to save the current videomem buffer
*   Return: None
*   Side Effect: change the content that des_addr points to (25 * 80 * 2)
*/
void copy_to_screen_buffer(uint8_t* source){
    memcpy((uint8_t*)video_mem, (uint8_t*)source, (NUM_COLS * NUM_ROWS) << 1);
}


/*  int get_row_stack()
*-----------------------------------------------------------------------------------
*   Description: Helper function that returns row_stack
*   Input: None
*   Return: row_stack (int_32t)
*   Side Effect: None
*/
int get_row_stack(){
    return row_stack;
}

/*  int get_column_stack()
*-----------------------------------------------------------------------------------
*   Description: Helper function that returns column stack
*   Input: None
*   Return: column_stack (int_32t)
*   Side Effect: None
*/
int get_column_stack(){
    return column_stack;
}

/*  int get_screen_x()
*-----------------------------------------------------------------------------------
*   Description: Helper function that returns screen_x
*   Input: None
*   Return: row_stack (int_32t)
*   Side Effect: None
*/
int get_screen_x(){
    return screen_x;
}

/*  int get_screen_y()
*-----------------------------------------------------------------------------------
*   Description: Helper function that returns screen_y
*   Input: None
*   Return: column_stack (int_32t)
*   Side Effect: None
*/
int get_screen_y(){
    return screen_y;
}

int set_row_stack_in(int rs){
    row_stack = rs;
    return 0;
}

int set_column_stack_in(int cs){
    column_stack = cs;
    return 0;
}

/*
 * pcb_t* get_curr_pcb()
 * Inputs: none
 * Return Value: pcb pointer
 * Function: get the current pcb pointer
 */
pcb_t* get_curr_pcb() {
    pcb_t * curr_pcb;
    // use bit mask to get the current pcb pointer
    asm volatile(
        "andl %%esp, %%eax;"
        : "=a" (curr_pcb)
        : "a"  (PCB_MASK)
        : "cc"
    );
    return curr_pcb;
}

/*
 * pcb_t* get_pcb_by_index(uint32_t index)
 * Inputs: index -- the process number
 * Return Value: pcb pointer
 * Function: get the pcb pointer corresponding to the process number
 */
pcb_t* get_pcb_by_index(uint32_t index) {
    return (pcb_t*)(EIGHT_MB_SIZE - (index + 1) * EIGHT_KB_SIZE);
}
