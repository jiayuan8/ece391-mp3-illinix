.data
	MIN = 1
	MAX = 11

.text

.global syscall_assembly

syscall_assembly:

	# save registers
	pushl	%esi
	pushl 	%edi
	pushl	%ebp

	# check if the system call number is within range
	cmpl	$MIN, %eax
	jl		error
	cmpl 	$MAX, %eax
	jg 		error

	# push arguments onto the stack
	pushl 	%edx
	pushl	%ecx
	pushl	%ebx

	decl	%eax		# let eax be 0 indexed
	# call the corresponding function
	call 	*jumptable(,%eax,4)

	# stack teardown
	popl 	%ebx
	popl 	%ecx
	popl 	%ebx

	jmp 	done

error:
	movl	$-1, %eax 	# return -1 if error

done:
	# restore saved registers
	popl 	%ebp
	popl 	%edi
	popl 	%esi

	iret

jumptable:
	.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn, play
