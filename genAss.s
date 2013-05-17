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
	.global	schedule
	.type	schedule, %function
schedule:
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr}
	str	r0, [fp, #-
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #31
	msr	cpsr_c, ip
	ldr	sp, [r0]
	ldmfd	sp, {r4, r5, r6, r7, r8, r9, r10, r11, sp, lr}
	movs	pc, lr
	.size	schedule, .-schedule
	.align	2
	.global syscall_enter
	.type	syscall_enter, %function
syscall_enter:
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #31
	msr	cpsr_c, ip
	mov	ip, sp
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
	mov	r4, sp
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	ldr	r3, [lr, #-4]
	ldmfd	sp, {fp, sp, lr}
	ldr	r2, [fp, #-20]
	str	r4, [r2]
	mov	pc, lr
	.size	syscall_enter, .-syscall_enter
