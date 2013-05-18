XCC = gcc
AS = as
LD = ld
CFLAGS = -c -fPIC -Wall -I. -Iinclude -mcpu=arm920t -msoft-float

ASFLAGS = -mcpu=arm920t -mapcs-32

LDFLAGS = -init main -Map a1.map -N -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L/u/wbcowan/cs452/io/lib -Llib

all: a1.s a1.elf

a1.s: a1.c include/a1.h include/task.h include/syscall.h
	$(XCC) -S $(CFLAGS) a1.c

a1.o: a1.s syscall.s genAss.s
	$(AS) $(ASFLAGS) -o a1.o a1.s syscall.s genAss.s

a1.elf: a1.o
	$(LD) $(LDFLAGS) -o $@ a1.o -lbwio -lgcc
	cp a1.elf /u/cs452/tftp/ARM/djgroot/a1.elf

clean:
	-rm -f a1.elf *.s *.o a1.map
