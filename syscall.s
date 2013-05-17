	.text
	.align	2
	.global	Exit
	.type	Exit, %function
Exit:
	swi	#0
	.size	Exit, .-Exit
