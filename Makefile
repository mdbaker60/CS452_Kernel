XCC = gcc
AS = as
LD = ld
CFLAGS = -c -O2 -fPIC -Wall -Ilib/include -Ikern/include -Iuser/include -mcpu=arm920t -msoft-float

ASFLAGS = -mcpu=arm920t -mapcs-32

LDFLAGS = -init main -Map bin/kern.map -N -T kern/orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L/u/wbcowan/cs452/io/lib

all: bin/kern.s bin/kern_a2.elf

bin/kern.s: kern/kern.c kern/include/kern.h kern/include/task.h kern/include/queue.h
	$(XCC) -S $(CFLAGS) kern/kern.c
	mv kern.s bin/kern.s

bin/queue.s: kern/queue.c kern/include/queue.h
	$(XCC) -S $(CFLAGS) kern/queue.c
	mv queue.s bin/queue.s

bin/string.s: lib/string.c lib/include/string.h
	$(XCC) -S $(CFLAGS) lib/string.c
	mv string.s bin/string.s

bin/clock.s: lib/clock.c lib/include/clock.h
	$(XCC) -S $(CFLAGS) lib/clock.c
	mv clock.s bin/clock.s

bin/prng.s: lib/prng.c lib/include/prng.h
	$(XCC) -S $(CFLAGS) lib/prng.c
	mv prng.s bin/prng.s

bin/userTasks.s: user/userTasks.c kern/include/syscall.h
	$(XCC) -S $(CFLAGS) user/userTasks.c
	mv userTasks.s bin/userTasks.s

bin/nameServer.s: user/nameServer.c kern/include/syscall.h
	$(XCC) -S $(CFLAGS) user/nameServer.c
	mv nameServer.s bin/nameServer.s

bin/kern.o: bin/kern.s kern/syscall.s kern/genAss.s
	$(AS) $(ASFLAGS) -o bin/kern.o bin/kern.s kern/syscall.s kern/genAss.s lib/memcpy.s

bin/queue.o: bin/queue.s
	$(AS) $(ASFLAGS) -o bin/queue.o bin/queue.s

bin/string.o: bin/string.s
	$(AS) $(ASFLAGS) -o bin/string.o bin/string.s

bin/clock.o: bin/clock.s
	$(AS) $(ASFLAGS) -o bin/clock.o bin/clock.s

bin/prng.o: bin/prng.s
	$(AS) $(ASFLAGS) -o bin/prng.o bin/prng.s

bin/nameServer.o: bin/nameServer.s
	$(AS) $(ASFLAGS) -o bin/nameServer.o bin/nameServer.s

bin/userTasks.o: bin/userTasks.s
	$(AS) $(ASFLAGS) -o bin/userTasks.o bin/userTasks.s

bin/kern_a2.elf: bin/kern.o bin/userTasks.o bin/queue.o bin/nameServer.o bin/string.o bin/clock.o bin/prng.o
	$(LD) $(LDFLAGS) -o bin/kern_a2.elf bin/string.o bin/kern.o bin/queue.o bin/nameServer.o bin/userTasks.o bin/clock.o bin/prng.o -lbwio -lgcc
	cp bin/kern_a2.elf /u/cs452/tftp/ARM/djgroot/kern_a3.elf
