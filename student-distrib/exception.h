#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "lib.h"
#include "x86_desc.h"
#include "syscall.h"

#define EXCEPTION_MAGIC 0x0F

// below are exception handlers
void divide_by_zero_exception();
void debug_exception();
void nmi_exception();
void breakpoint_exception();
void overflow_exception();
void bound_range_exception();
void invalid_opcode_exception();
void device_not_available_exception();
void double_fault_exception();
void coprocessor_exception();
void invalid_tss_exception();
void segment_not_present_exception();
void stack_fault_exception();
void general_protection_exception();
void page_fault_exception();
void fpu_floating_point_exception();
void alignment_check_exception();
void machine_check_exception();
void simd_floating_point_exception();

// function to print the message to the screen
void print_exception(int8_t * str);

#endif
