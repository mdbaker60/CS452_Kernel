	.text
	.align	2
	.global	getSP
	.type	getSP, %function
getSP:
	mov	r0, sp
	mov	pc, lr
	.size	getSP, .-getSP
	.align	2
	.global	getNextRequest
	.type	getNextRequest, %function
getNextRequest:
	mov	ip, sp
	stmfd	sp!, {r0, r1, fp, ip, lr}
	ldr	ip, [r0, #4]
	msr	spsr, ip
	ldr	lr, [r0, #8]
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #31
	msr	cpsr_c, ip
	ldr	sp, [r0]
	ldmfd	sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, sp, lr}
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	movs	pc, lr
	.size	getNextRequest, .-getNextRequest
	.align	2
	.global syscall_enter
	.type	syscall_enter, %function
syscall_enter:
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #31
	msr	cpsr_c, ip
	mov	ip, sp
	stmfd	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	mov	r2, sp
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	mov	r3, lr
	ldmfd	sp, {r0, r1, fp, sp, lr}
	str	r2, [r0]
	mrs	ip, spsr
	str	ip, [r0, #4]
	str	r3, [r0, #8]
	mov	ip, r3
	ldr	r3, [r3, #-4]
	and	ip, r3, #0xFF
	str	ip, [r1]
	and	ip, r3, #0x700
	mov	ip, ip, lsr #8
	add	r1, r1, #4
	teq	ip, #5
	bne	__syscall_enter_arguments_bottom__
	ldr	r0, [r2, #48]
	ldr	r0, [r0, #8]
	str	r0, [r1, #16]
	sub	ip, ip, #1
	b	__syscall_enter_arguments_bottom__
__syscall_enter_arguments_top__:
	ldr	r0, [r2]
	str	r0, [r1]
	add	r2, r2, #4
	add	r1, r1, #4
	sub	ip, ip, #1
__syscall_enter_arguments_bottom__:
	teq	ip, #0x0
	bne	__syscall_enter_arguments_top__
	mov	pc, lr
	.size	syscall_enter, .-syscall_enter
