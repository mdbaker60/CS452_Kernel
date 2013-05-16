	.file	"a1.c"
	.section	.rodata
	.align	2
.LC0:
	.ascii	"this is a test\015\000"
	.text
	.align	2
	.global	sub
	.type	sub, %function
sub:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {sl, fp, ip, lr, pc}
	sub	fp, ip, #4
	ldr	sl, .L4
.L3:
	add	sl, pc, sl
	mov	r0, #1
	ldr	r3, .L4+4
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	bl	Exit(PLT)
	ldmfd	sp, {sl, fp, sp, pc}
.L5:
	.align	2
.L4:
	.word	_GLOBAL_OFFSET_TABLE_-(.L3+8)
	.word	.LC0(GOTOFF)
	.size	sub, .-sub
	.section	.rodata
	.align	2
.LC1:
	.ascii	"main entered\015\000"
	.align	2
.LC2:
	.ascii	"software interupt setup\015\000"
	.align	2
.LC3:
	.ascii	"stack pointer retrieved\015\000"
	.align	2
.LC4:
	.ascii	"memory setup\015\000"
	.align	2
.LC5:
	.ascii	"task created\015\000"
	.align	2
.LC6:
	.ascii	"task completed\015\000"
	.text
	.align	2
	.global	main
	.type	main, %function
main:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {sl, fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #4
	ldr	sl, .L9
.L8:
	add	sl, pc, sl
	mov	r0, #1
	ldr	r3, .L9+4
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	mov	r2, #40
	ldr	r3, .L9+8
	ldr	r3, [sl, r3]
	str	r3, [r2, #0]
	mov	r0, #1
	ldr	r3, .L9+12
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	bl	getSP(PLT)
	mov	r2, r0
	ldr	r3, .L9+16
	ldr	r3, [sl, r3]
	str	r2, [r3, #0]
	mov	r0, #1
	ldr	r3, .L9+20
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	ldr	r3, .L9+16
	ldr	r3, [sl, r3]
	ldr	r3, [r3, #0]
	sub	r2, r3, #4096
	ldr	r3, .L9+24
	ldr	r3, [sl, r3]
	str	r2, [r3, #0]
	mov	r0, #1
	ldr	r3, .L9+28
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	mov	r0, #0
	ldr	r3, .L9+32
	ldr	r3, [sl, r3]
	mov	r1, r3
	bl	Create_sys(PLT)
	mov	r3, r0
	str	r3, [fp, #-20]
	mov	r0, #1
	ldr	r3, .L9+36
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	ldr	r0, [fp, #-20]
	bl	schedule(PLT)
	mov	r3, r0
	str	r3, [fp, #-20]
	mov	r0, #1
	ldr	r3, .L9+40
	add	r3, sl, r3
	mov	r1, r3
	bl	bwprintf(PLT)
	mov	r3, #0
	mov	r0, r3
	ldmfd	sp, {r3, sl, fp, sp, pc}
.L10:
	.align	2
.L9:
	.word	_GLOBAL_OFFSET_TABLE_-(.L8+8)
	.word	.LC1(GOTOFF)
	.word	syscall_enter(GOT)
	.word	.LC2(GOTOFF)
	.word	kernMemStart(GOT)
	.word	.LC3(GOTOFF)
	.word	freeMemStart(GOT)
	.word	.LC4(GOTOFF)
	.word	sub(GOT)
	.word	.LC5(GOTOFF)
	.word	.LC6(GOTOFF)
	.size	main, .-main
	.align	2
	.global	Create_sys
	.type	Create_sys, %function
Create_sys:
	@ args = 0, pretend = 0, frame = 12
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {sl, fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #12
	ldr	sl, .L14
.L13:
	add	sl, pc, sl
	str	r0, [fp, #-24]
	str	r1, [fp, #-28]
	ldr	r3, .L14+4
	ldr	r3, [sl, r3]
	ldr	r3, [r3, #0]
	str	r3, [fp, #-20]
	ldr	r3, .L14+4
	ldr	r3, [sl, r3]
	ldr	r3, [r3, #0]
	sub	r2, r3, #240
	ldr	r3, .L14+4
	ldr	r3, [sl, r3]
	str	r2, [r3, #0]
	ldr	r3, .L14+4
	ldr	r3, [sl, r3]
	ldr	r3, [r3, #0]
	mov	r2, r3
	ldr	r3, [fp, #-20]
	str	r2, [r3, #48]
	ldr	r2, [fp, #-28]
	ldr	r3, [fp, #-20]
	str	r2, [r3, #52]
	ldr	r3, .L14+4
	ldr	r3, [sl, r3]
	ldr	r3, [r3, #0]
	sub	r2, r3, #4096
	ldr	r3, .L14+4
	ldr	r3, [sl, r3]
	str	r2, [r3, #0]
	ldr	r3, [fp, #-20]
	mov	r0, r3
	sub	sp, fp, #16
	ldmfd	sp, {sl, fp, sp, pc}
.L15:
	.align	2
.L14:
	.word	_GLOBAL_OFFSET_TABLE_-(.L13+8)
	.word	freeMemStart(GOT)
	.size	Create_sys, .-Create_sys
	.bss
	.align	2
freeMemStart:
	.space	4
	.align	2
kernMemStart:
	.space	4
	.align	2
curTask:
	.space	4
	.ident	"GCC: (GNU) 4.0.2"
