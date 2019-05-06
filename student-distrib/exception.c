#include "exception.h"

void divide_by_zero_exception() {
    print_exception("EXCEPTION: divide_by_zero_exception!\n");
}
void debug_exception() {
    print_exception("EXCEPTION: debug_exception!\n");
}
void nmi_exception() {
    print_exception("EXCEPTION: nmi_exception!\n");
}
void breakpoint_exception() {
    print_exception("EXCEPTION: breakpoint_exception!\n");
}
void overflow_exception() {
    print_exception("EXCEPTION: overflow_exception!\n");
}
void bound_range_exception() {
    print_exception("EXCEPTION: bound_range_exception!\n");
}
void invalid_opcode_exception() {
    print_exception("EXCEPTION: invalid_opcode_exception!\n");
}
void device_not_available_exception() {
    print_exception("EXCEPTION: device_not_available_exception!\n");
}
void double_fault_exception() {
    print_exception("EXCEPTION: double_fault_exception!\n");
}
void coprocessor_exception() {
    print_exception("EXCEPTION: coprocessor_exception!\n");
}
void invalid_tss_exception() {
    print_exception("EXCEPTION: invalid_tss_exception!\n");
}
void segment_not_present_exception() {
    print_exception("EXCEPTION: segment_not_present_exception!\n");
}
void stack_fault_exception() {
    print_exception("EXCEPTION: stack_fault_exception!\n");
}
void general_protection_exception() {
    print_exception("EXCEPTION: general_protection_exception!\n");
}
void page_fault_exception() {
    uint32_t * addr;
    uint32_t  addr1;
    uint32_t val;
    asm volatile(
        "movl %%cr2, %%eax;"
        "movl %%esp, %%ebx"
        : "=a" (addr),  "=b" (addr1)
        :
        : "cc"
    );
    val = *(uint32_t*)((uint32_t)(addr1) - 12);
    printf("exception occur at address: 0x%x\n", addr);
    printf("user esp at address: 0x%x\n", val);
    print_exception("EXCEPTION: page_fault_exception!\n");
}
void fpu_floating_point_exception() {
    print_exception("EXCEPTION: fpu_floating_point_exception!\n");
}
void alignment_check_exception() {
    print_exception("EXCEPTION: alignment_check_exception!\n");
}
void machine_check_exception() {
    print_exception("EXCEPTION: machine_check_exception!\n");
}
void simd_floating_point_exception() {
    print_exception("EXCEPTION: simd_floating_point_exception!\n");
}

/* void print_exception(int8_t * str)
 * Inputs: int8_t * str -- the string to be printed on the screen
 * Return Value: void
 * Function: print the exception message */
void print_exception(int8_t * str) {
    //clear();            // clear the screen
    //blue_screen();      // turn the screen to blue
    printf(str);          // print the message
    while(1);
    // squash user level progrma and return to shell
    halt(EXCEPTION_MAGIC);          // 256 indicates the program dies by an exception
    //while(1);
}
