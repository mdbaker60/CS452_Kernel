	.align	2
	.global	memcpy
	.type	memcpy, %function
memcpy:
	mov	ip, sp
	stmfd	sp!, {r0, r3 - r11, ip, lr}
	and	r3, r1, #3
	and 	r4, r0, #3
	cmp	r3, r4
	bne	__smemcopy_byte__
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
__smemcopy_byte__:
	cmp	r3, #4
	beq	__smemcopy32__
	ldrb	r4, [r1]
	strb	r4, [r0]
	add	r1, r1, #1
	add	r0, r0, #1
	add	r3, r3, #1
	sub	r2, r2, #1
	b	__smemcopy_byte__
__smemcopy32__:
	cmp	r2, #32
	blt	__smemcopy16__
	ldmia	r1!, {r3 - r10}
	bl	__write32__
	sub	r2, r2, #32
	b	__smemcopy32__
__smemcopy16__:
	cmp	r2, #16
	blt	__smemcopy8__
	ldmia	r1!, {r7 - r10}
	bl	__write16__
	sub	r2, r2, #16
__smemcopy8__:
	cmp	r2, #8
	blt	__smemcopy4__
	ldmia	r1!, {r9, r10}
	bl	__write8__
	sub	r2, r2, #8
__smemcopy4__:
	cmp	r2, #4
	blt	__smemcopy2__
	ldr	r10, [r1]
	bl	__write4__
	add	r1, r1, #4
	sub	r2, r2, #4
__smemcopy2__:
	cmp	r2, #0
	beq	__memcopy_done__
	ldrb	r3, [r1]
	strb	r3, [r0]
	add	r1, r1, #1
	add	r0, r0, #1
	sub	r2, r2, #1
	b	__smemcopy2__
__write32__:
	mov	ip, sp
	stmfd	sp!, {ip, lr}
	b	__write32_enter__
__write16__:
	mov	ip, sp
	stmfd	sp!, {ip, lr}
	b	__write16_enter__
__write8__:
	mov	ip, sp
	stmfd	sp!, {ip, lr}
	b	__write8_enter__
__write4__:
	mov	ip, sp
	stmfd	sp!, {ip, lr}
	b	__write4_enter__
__write32_enter__:
	mov	r11, r3
	bl	__writebyte__
	mov	r11, r4
	bl	__writebyte__
	mov	r11, r5
	bl	__writebyte__
	mov	r11, r6
	bl	__writebyte__
__write16_enter__:
	mov	r11, r7
	bl	__writebyte__
	mov	r11, r8
	bl	__writebyte__
__write8_enter__:
	mov	r11, r9
	bl	__writebyte__
__write4_enter__:
	mov	r11, r10
	bl	__writebyte__
	ldmfd	sp, {sp, pc}
__writebyte__:
	strb	r11, [r0]
	mov	r11, r11, lsr #8
	strb	r11, [r0, #1]
	mov	r11, r11, lsr #8
	strb	r11, [r0, #2]
	mov	r11, r11, lsr #8
	strb	r11, [r0, #3]
	add	r0, r0, #4
	mov	pc, lr
__memcopy_done__:
	ldmfd	sp, {r0, r3 - r11, sp, pc}
	.size	memcpy, .-memcpp
