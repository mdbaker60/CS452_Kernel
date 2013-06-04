	.text
	.align	2
	.global	Create
	.type	Create, %function
Create:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x201
	ldmfd	sp, {sp, pc}
	.size	Create, .-Create
	.align	2
	.global	MyTid
	.type	MyTid, %function
MyTid:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x2
	ldmfd	sp, {sp, pc}
	.size	MyTid, .-MyTid
	.align	2
	.global	MyParentTid
	.type	MyParentTid, %function
MyParentTid:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x3
	ldmfd	sp, {sp, pc}
	.size	MyParentTid, .-MyParentTid
	.align	2
	.global	Pass
	.type	Pass, %function
Pass:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x4
	ldmfd	sp, {sp, pc}
	.size	Pass, .-Pass
	.align	2
	.global	Exit
	.type	Exit, %function
Exit:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x5
	ldmfd	sp, {sp, pc}
	.size	Exit, .-Exit
	.align	2
	.global	Send
	.type	Send, %function
Send:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x506
	ldmfd	sp, {sp, pc}
	.size	Send, .-Send
	.align	2
	.global	Receive
	.type	Receive, %function
Receive:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x307
	ldmfd	sp, {sp, pc}
	.size	Receive, .-Receive
	.align	2
	.global	Reply
	.type	Reply, %function
Reply:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x308
	ldmfd	sp, {sp, pc}
	.size	Reply, .-Reply
	.align	2
	.global	AwaitEvent
	.type	AwaitEvent, %function
AwaitEvent:
	mov	ip, sp
	stmfd	sp!, {sp, lr}
	swi	#0x109
	ldmfd	sp, {sp, pc}
	.size	AwaitEvent, .-AwaitEvent
