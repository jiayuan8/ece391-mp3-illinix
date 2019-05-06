#include "lib.h"
#include "types.h"
#include "keyboard.h"
#include "rtc.h"
#include "paging.h"
#include "syscall.h"
#include "sb16.h"

#ifndef TERMINAL_H
#define TERMINAL_H


// driver function for terminal
extern int32_t terminal_open(const uint8_t* filename);
extern int32_t terminal_close(int32_t fd);
extern int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

extern int32_t switch_to_terminal(uint8_t terminal_id);
extern int32_t save_terminal_info(uint8_t terminal_id);
extern int32_t restore_terminal_info(uint8_t terminal_id);

// Helper functions here
void init_terminal();
uint8_t is_running(uint8_t terminal_id);





#endif
