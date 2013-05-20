XCC = gcc
AS = as
LD = ld
CFLAGS = -c -fPIC -Wall -Ikern/include -Iuser/include -mcpu=arm920t -msoft-float

ASFLAGS = -mcpu=arm920t -mapcs-32

LDFLAGS = -init main -Map bin/a0.map -N -T kern/orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L/u/wbcowan/cs452/io/lib

all: bin/a1.s bin/a1.elf

bin/a1.s: kern/a1.c kern/include/a1.h kern/include/task.h
	$(XCC) -S $(CFLAGS) kern/a1.c
	mv a1.s bin/a1.s

bin/userTasks.s: user/userTasks.c kern/include/syscall.h
	$(XCC) -S $(CFLAGS) user/userTasks.c
	mv userTasks.s bin/userTasks.s

bin/a1.o: bin/a1.s kern/syscall.s kern/genAss.s
	$(AS) $(ASFLAGS) -o bin/a1.o bin/a1.s kern/syscall.s kern/genAss.s

bin/userTasks.o: bin/userTasks.s
	$(AS) $(ASFLAGS) -o bin/userTasks.o bin/userTasks.s

bin/a1.elf: bin/a1.o bin/userTasks.o
	$(LD) $(LDFLAGS) -o bin/a1.elf bin/a1.o bin/userTasks.o -lbwio -lgcc
	cp bin/a1.elf /u/cs452/tftp/ARM/djgroot/a1.elf
