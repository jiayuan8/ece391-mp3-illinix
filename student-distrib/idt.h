#ifndef IDT_H
#define IDT_H

#include "x86_desc.h"
#include "exception.h"
#include "int_linkage.h"
#include "keyboard.h"
#include "rtc.h"
#include "sb16.h"
#include "scheduling.h"
#include "syscall_linkage.h"
#include "syscall.h"

#define IDT_DIVISION	0x00
#define IDT_DEBUG		0x01
#define IDT_NMI			0x02
#define IDT_BREAKPOINT	0x03
#define IDT_OVERFLOW	0x04
#define IDT_BOUND_RANGE	0x05
#define IDT_INV_OPCODE	0x06
#define IDT_DEV_NOAVL	0x07
#define IDT_DOUBLE_FA	0x08
#define IDT_COPROCESSOR	0x09
#define IDT_INALID_TSS	0x0A
#define IDT_SEG_NOPRE	0x0B
#define IDT_STACK_FA	0x0C
#define IDT_GEN_PROTEC	0x0D
#define IDT_PAGE_FAULT	0x0E
#define IDT_FLOAT_POINT 0x10
#define IDT_ALIGNMENT   0x11
#define IDT_MACHINE     0x12
#define IDT_SIMD        0x13
#define IDT_LAST        IDT_SIMD+1

#define IDT_PIT         0x20
#define IDT_KEYBOARD 	0x21
#define IDT_SB16        0x25
#define IDT_RTC 		0x28
#define IDT_SYS_CALL	0x80

// function to initialize interrupt discriptor table
extern int init_idt();

#endif
