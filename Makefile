XCC = gcc
AS = as
LD = ld
CFLAGS = -c -fPIC -Wall -Ikern/include -Iuser/include -mcpu=arm920t -msoft-float

ASFLAGS = -mcpu=arm920t -mapcs-32

LDFLAGS = -init main -Map bin/kern.map -N -T kern/orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L/u/wbcowan/cs452/io/lib

all: bin/kern.s bin/kern_a2.elf

bin/kern.s: kern/kern.c kern/include/kern.h kern/include/task.h kern/include/queue.h
	$(XCC) -S $(CFLAGS) kern/kern.c
	mv kern.s bin/kern.s

bin/queue.s: kern/queue.c kern/include/queue.h
	$(XCC) -S $(CFLAGS) kern/queue.c
	mv queue.s bin/queue.s

bin/userTasks.s: user/userTasks.c kern/include/syscall.h
	$(XCC) -S $(CFLAGS) user/userTasks.c
	mv userTasks.s bin/userTasks.s

bin/kern.o: bin/kern.s kern/syscall.s kern/genAss.s
	$(AS) $(ASFLAGS) -o bin/kern.o bin/kern.s kern/syscall.s kern/genAss.s

bin/queue.o: bin/queue.s
	$(AS) $(ASFLAGS) -o bin/queue.o bin/queue.s

bin/userTasks.o: bin/userTasks.s
	$(AS) $(ASFLAGS) -o bin/userTasks.o bin/userTasks.s

bin/kern_a2.elf: bin/kern.o bin/userTasks.o bin/queue.o
	$(LD) $(LDFLAGS) -o bin/kern_a2.elf bin/kern.o bin/queue.o bin/userTasks.o -lbwio -lgcc
	cp bin/kern_a2.elf /u/cs452/tftp/ARM/djgroot/kern_a2.elf
