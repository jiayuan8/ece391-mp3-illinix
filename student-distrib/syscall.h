#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "paging.h"
#include "filesystem.h"
#include "rtc.h"
#include "terminal.h"
#include "x86_desc.h"
#include "exception.h"
#include "lib.h"

#define IN_USE          1
#define NOT_IN_USE      0

#define RTC_TYPE        0
#define DIR_TYPE        1
#define REGULAR_FILE    2

#define SPACE           ' '
#define FOUR_BYTES      4
#define MAGIC_ONE       0x7F            // the four magic numbers
#define MAGIC_TWO       0x45
#define MAGIC_THREE     0x4C
#define MAGIC_FOUR      0x46
#define ENTRY_POINT     24              // offset to entry point in the executable
#define EXCEPTION_RET   256             // the number to be returned when exception occur


extern void send_signal(uint8_t signum);
extern void signal_default(uint8_t signum);



// system calls
int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);
int32_t play(const uint8_t* filename);

// this function should never be called
int32_t fail();
// helper function
extern int32_t get_pid();

uint8_t get_terminal_id(uint32_t pid);

#endif
