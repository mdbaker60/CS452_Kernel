	.text
	.align	2
	.global	getSP
	.type	getSP, %function
getSP:
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	mov	r0, ip
	ldmfd	sp, {fp, sp, pc}
	.size	getSP, .-getSP
	.align	2
	.global syscall_enter
	.type	syscall_enter, %function
syscall_enter:
	mrs	r0, cpsr
	bic	r0, r0, #0x1F
	orr	r0, r0, #31
	msr	cpsr_c, r0
	mov	ip, sp
	stmfd	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
	mov	r0, ip
	mrs	r0, cpsr
	bic	r0, r0, #0x1F
	orr	r0, r0, #0x13
	msr	cpsr_c, r0
	ldr	r3, [lr, #-4]
	ldmfd	sp, {fp, sp, lr}
	mov	pc, lr
	.size syscall_enter, .-syscall_enter
	.align	2
	.global	schedule
	.type	schedule, %function
schedule:
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr}
	mov	r2, r0
	mrs	r0, cpsr
	orr	r0, r0, #0x1F
	msr	cpsr_c, r0
	ldmfd	r2, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp, lr}
	mrs	r0, cpsr
	bic	r0, r0, #0x1F
	orr	r0, r0, #0x10
	msr	cpsr_c, r0
	mov	pc, lr
	.size schedule, .-schedule
