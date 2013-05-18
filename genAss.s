	.text
	.align	2
	.global	getSP
	.type	getSP, %function
getSP:
	mov	r0, sp
	mov	pc, lr
	.size	getSP, .-getSP
	.align	2
	.global	schedule
	.type	schedule, %function
schedule:
	mov	ip, sp
	stmfd	sp!, {r0, r1, fp, ip, lr}
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #31
	msr	cpsr_c, ip
	ldr	sp, [r0]
	ldmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, sp, lr}
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
	stmfd	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ip, lr}
	mov	r2, sp
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	ldr	r3, [lr, #-4]
	and	r3, r3, #0xFF
	ldmfd	sp, {r0, r1, fp, sp, lr}
	str	r2, [r0]
	str	r3, [r1]
	mov	pc, lr
	.size	syscall_enter, .-syscall_enter