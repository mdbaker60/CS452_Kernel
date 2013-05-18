	.text
	.align	2
	.global	Exit
	.type	Exit, %function
Exit:
	swi	#0
	.size	Exit, .-Exit
	.align	2
	.global	syscall
	.type	syscall, %function
syscall:
	swi	#1
	.size	syscall, .-syscall
