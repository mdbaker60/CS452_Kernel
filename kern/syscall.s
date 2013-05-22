	.text
	.align	2
	.global	Create
	.type	Create, %function
Create:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x300
	ldmfd	sp, {sp, pc}
	.size	Create, .-Create
	.align	2
	.global	MyTid
	.type	MyTid, %function
MyTid:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x1
	ldmfd	sp, {sp, pc}
	.size	MyTid, .-MyTid
	.align	2
	.global	MyParentTid
	.type	MyParentTid, %function
MyParentTid:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x2
	ldmfd	sp, {sp, pc}
	.size	MyParentTid, .-MyParentTid
	.align	2
	.global	Pass
	.type	Pass, %function
Pass:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x3
	ldmfd	sp, {sp, pc}
	.size	Pass, .-Pass
	.align	2
	.global	Exit
	.type	Exit, %function
Exit:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x4
	ldmfd	sp, {sp, pc}
	.size	Exit, .-Exit
