/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0
#define MIN_FD          2           // minimum file discriptor number
#define MAX_FD          8           // maximum file discriptor number
#define ARG_MAX         100
#define SCREEN_COLUMN   80
#define SCREEN_ROW      25
#define KEY_ARR_SIZE_OLD    128

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef struct
{
    uint8_t f_name[32];
    uint32_t f_type;
    uint32_t i_node;
    uint8_t reserved[24];
}dentry_t;

typedef struct {
    int32_t (*open)(const uint8_t* filename);
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*close)(int32_t fd);
} file_operation_ptrs;

typedef struct {
    file_operation_ptrs *ptrs;
    uint32_t inode;
    uint32_t file_pos;
    uint32_t flags;
} file_desc_t;

typedef struct {
    file_desc_t files[MAX_FD];
    uint32_t pid;
    uint32_t parent_pid;
    uint32_t parent_esp;
    uint32_t parent_ebp;
    uint32_t schedule_esp;
    uint32_t schedule_ebp;
    uint8_t argument[ARG_MAX];
    uint32_t pending_signal;
    void (*sighandler)(uint8_t);
} pcb_t;

typedef struct {
    uint8_t terminal_id;
    uint8_t screen_pos_x;
    uint8_t screen_pos_y;
    uint8_t key_arr_ptr_old;
    uint8_t key_arr_overflow_old;
    uint8_t row_stack_old;
    uint8_t column_stack_old;
    uint8_t key_arr_old[KEY_ARR_SIZE_OLD + 1];
    uint8_t* screen_buffer;
} terminal_t;

#endif /* ASM */

#endif /* _TYPES_H */
