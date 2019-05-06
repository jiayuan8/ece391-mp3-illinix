.text

.global	keyboard_assembly
.global rtc_assembly
.global pit_assembly
.global sb16_assembly

keyboard_assembly:
	pushal		#push all the registers
	pushfl		#push all the flags

	call keyboard_handler

	popfl		#pop all the flags
	popal		#pop all the registers

	iret

rtc_assembly:
	pushal		#push all the registers
	pushfl		#push all the flags

	call rtc_handler

	popfl		#pop all the flags
	popal		#pop all the registers

	iret

pit_assembly:
	pushal		#push all the registers
	pushfl		#push all the flags

	call pit_handler

	popfl		#pop all the flags
	popal		#pop all the registers

	iret

sb16_assembly:
	pushal		#push all the registers
	pushfl		#push all the flags

	call sb16_handler

	popfl		#pop all the flags
	popal		#pop all the registers

	iret