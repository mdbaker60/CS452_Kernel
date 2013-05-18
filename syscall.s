	.text
	.align	2
	.global	Exit
	.type	Exit, %function
Exit:
	swi	#0x0
	.size	Exit, .-Exit
	.align	2
	.global	MyTid
	.type	MyTid, %function
MyTid:
	swi	#0x1
	.size	MyTid, .-MyTid
