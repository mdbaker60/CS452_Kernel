	.text
	.align	2
	.global	getSP
	.type	getSP, %function
getSP:
	mov	r0, sp
	mov	pc, lr
	.size	getSP, .-getSP
	.align	2
	.global	setIRQ_SP
	.type 	setIRQ_SP, %function	
setIRQ_SP:
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x12
	msr	cpsr_c, ip
	mov	sp, r0
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	mov	pc, lr
	.size	setIRQ_SP, .-setIRQ_SP
	.align	2
	.global	getNextRequest
	.type	getNextRequest, %function
getNextRequest:
	mov	ip, sp
	stmfd	sp!, {r0, r1, r4 - r10, fp, ip, lr}
	ldr	ip, [r0, #4]
	msr	spsr, ip
	ldr	lr, [r0, #8]
	mrs	ip, cpsr
	orr	ip, ip, #0x1F
	msr	cpsr_c, ip
	ldr	sp, [r0]
	ldr	ip, [sp, #56]
	mrs	r0, cpsr
	bic	r0, r0, #0x1F
	orr	r0, r0, #0x13
	msr	cpsr_c, r0
	str	ip, [sp, #-4]
	orr	r0, r0, #0x1F
	msr	cpsr_c, r0
	ldmfd	sp, {r0 - r10, fp, sp, lr}
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	ldr	ip, [sp, #-4]
	movs	pc, lr
	.size	getNextRequest, .-getNextRequest
	.align	2
	.global	int_enter
	.type	int_enter, %function
int_enter:
	str	ip, [sp, #-4]
	mrs	ip, cpsr
	orr	ip, ip, #0x1F
	msr	cpsr_c, ip
	mov	ip, sp
	sub	sp, sp, #4
	stmfd	sp!, {r0 - r10, fp, ip, lr}
	mrs	r0, cpsr
	bic	r0, r0, #0x1F
	orr	r0, r0, #0x12
	msr	cpsr_c, r0
	ldr	ip, [sp, #-4]
	orr	r0, r0, #0x1F
	msr	cpsr_c, r0
	str	ip, [sp, #56]
	mov	r2, sp
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	ldmfd	sp, {r0, r1, r4 - r10, fp, sp, lr}
	str	r2, [r0]
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x12
	msr	cpsr_c, ip
	mrs	r2, spsr
	str	r2, [r0, #4]
	sub	lr, lr, #4
	str	lr, [r0, #8]
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	mov	ip, #0
	str	ip, [r1]
	mov	pc, lr
	.size	int_enter, .-int_enter
	.align	2
	.global syscall_enter
	.type	syscall_enter, %function
syscall_enter:
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #31
	msr	cpsr_c, ip
	mov	ip, sp
	sub	sp, sp, #4
	stmfd	sp!, {r0 - r10, fp, ip, lr}
	mov	r2, sp
	mrs	ip, cpsr
	bic	ip, ip, #0x1F
	orr	ip, ip, #0x13
	msr	cpsr_c, ip
	mov	r3, lr
	ldmfd	sp, {r0, r1, r4 - r10, fp, sp, lr}
	str	r2, [r0]
	mrs	ip, spsr
	str	ip, [r0, #4]
	str	r3, [r0, #8]
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
	.align	2
	.global	enableCache
	.type	enableCache, %function
enableCache:
	mrc	p15, 0, r1, c1, c0, 0
	orr	r1, r1, #0x4
	orr	r1, r1, #0x1000
	mov	r0, #0
	mcr	p15, 0, r0, c5, c0, 0
	mcr	p15, 0, r0, c6, c0, 0
	mcr	p15, 0, r1, c1, c0, 0
	mov	pc, lr
	.size	enableCache, .-enableCache
