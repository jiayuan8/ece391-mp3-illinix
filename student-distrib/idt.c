#include "idt.h"

/* void init_idt();
 * Inputs: none
 * Return Value: 0 on success
 * Function: initialize the idt by setting the interrupt vectors into
 * 			 the interrupt discriptor table and set the appropriate gate
 */
int init_idt() {
	int i;					// loop index

	// loop through all the interrupt vectors
	for (i = 0; i < NUM_VEC; i++) {
		// select the kernel code segment
		idt[i].seg_selector = KERNEL_CS;
		// set the interrupt gate parameter
		// the four reserved fields are: 01100
		idt[i].reserved4 = 0;
		idt[i].reserved3 = 0;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].size = 1;			// indicate that it's 32bits
		idt[i].reserved0 = 0;
		idt[i].dpl = 0;				// kernel code has a privilege level of 0
		idt[i].present = 0;			// indicate the segment is present

		if (i < IDT_LAST) {
			idt[i].present = 1;
			idt[i].reserved3 = 1;
		}

		if (i == IDT_SYS_CALL) {
			idt[i].present = 1;
			idt[i].reserved3 = 1;
			idt[i].dpl = 3;			// indicate that's a user triggered
		}

		// if the current index is an interrupt
		if (i == IDT_PIT || i == IDT_RTC || i == IDT_KEYBOARD || i == IDT_SB16) {
			// set present
			idt[i].present = 1;
		}

	}

	// set the first 15 interrupt vector
	SET_IDT_ENTRY(idt[IDT_DIVISION], divide_by_zero_exception);
	SET_IDT_ENTRY(idt[IDT_DEBUG], debug_exception);
	SET_IDT_ENTRY(idt[IDT_NMI], nmi_exception);
	SET_IDT_ENTRY(idt[IDT_BREAKPOINT], breakpoint_exception);
	SET_IDT_ENTRY(idt[IDT_OVERFLOW], overflow_exception);
	SET_IDT_ENTRY(idt[IDT_BOUND_RANGE], bound_range_exception);
	SET_IDT_ENTRY(idt[IDT_INV_OPCODE], invalid_opcode_exception);
	SET_IDT_ENTRY(idt[IDT_DEV_NOAVL], device_not_available_exception);
	SET_IDT_ENTRY(idt[IDT_DOUBLE_FA], double_fault_exception);
	SET_IDT_ENTRY(idt[IDT_COPROCESSOR], coprocessor_exception);
	SET_IDT_ENTRY(idt[IDT_INALID_TSS], invalid_tss_exception);
	SET_IDT_ENTRY(idt[IDT_SEG_NOPRE], segment_not_present_exception);
	SET_IDT_ENTRY(idt[IDT_STACK_FA], stack_fault_exception);
	SET_IDT_ENTRY(idt[IDT_GEN_PROTEC], general_protection_exception);
	SET_IDT_ENTRY(idt[IDT_PAGE_FAULT], page_fault_exception);
	SET_IDT_ENTRY(idt[IDT_FLOAT_POINT], fpu_floating_point_exception);
	SET_IDT_ENTRY(idt[IDT_ALIGNMENT], alignment_check_exception);
	SET_IDT_ENTRY(idt[IDT_MACHINE], machine_check_exception);
	SET_IDT_ENTRY(idt[IDT_SIMD], simd_floating_point_exception);

	// set the pit idt
	SET_IDT_ENTRY(idt[IDT_PIT], pit_assembly);
	// set the keyboard idt
	SET_IDT_ENTRY(idt[IDT_KEYBOARD], keyboard_assembly);
	// set the sb16 idt
	SET_IDT_ENTRY(idt[IDT_SB16], sb16_assembly);
	// set the RTC idt
	SET_IDT_ENTRY(idt[IDT_RTC], rtc_assembly);

	// set the system call idt
	SET_IDT_ENTRY(idt[IDT_SYS_CALL], syscall_assembly);

	// load the idt
	lidt(idt_desc_ptr);
	return 0;
}
