	.align	2
	.global	memcpy_aligned
	.type	memcpy_aligned, %function
memcpy_aligned:
	mov	ip, sp
	stmfd	sp!, {r0, r3 - r10, ip, lr}
	and	r3, r1, #3
__memcopy_byte:
	cmp 	r3, #4
	beq 	__memcopy32__
	ldrb	r4, [r1]
	strb	r4, [r0]
	add	r1, r1, #1
	add	r0, r0, #1
	add	r3, r3, #1
	sub	r2, r2, #1
	b	__memcopy_byte
__memcopy32__:
	cmp	r2, #32
	blt	__memcopy16__
	ldmia	r1!, {r3 - r10}
	stmia	r0!, {r3 - r10}
	sub	r2, r2, #32
	b	__memcopy32__
__memcopy16__:
	cmp	r2, #16
	blt	__memcopy8__
	ldmia	r1!, {r3 - r6}
	stmia	r0!, {r3 - r6}
	sub	r2, r2, #16
__memcopy8__:
	cmp	r2, #8
	blt	__memcopy4__
	ldmia	r1!, {r3, r4}
	stmia	r0!, {r3, r4}
	sub	r2, r2, #8
__memcopy4__:
	cmp	r2, #4
	blt	__memcopy2__
	ldr	r3, [r1]
	str	r3, [r0]
	add	r1, r1, #4
	add	r0, r0, #4
	sub	r2, r2, #4
__memcopy2__:
	cmp	r2, #0
	beq	__memcopy_done__
	ldrb	r3, [r1]
	strb	r3, [r0]
	add	r1, r1, #1
	add	r0, r0, #1
	sub	r2, r2, #1
	b	__memcopy2__
__memcopy_done__:
	ldmfd	sp, {r0, r3 - r10, sp, pc}
	.size	memcpy_aligned, .-memcpy_aligned
