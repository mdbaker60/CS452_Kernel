XCC = gcc
AS = as
CFLAGS = -c -O2 -DDEBUG -fPIC -Wall -I../lib/include -Iinclude -I../user/include -mcpu=arm920t -msoft-float

ASFLAGS = -mcpu=arm920t -mapcs-32

all: ../bin/kern.o ../bin/queue.o

../bin/kern.s: kern.c include/kern.h include/task.h include/queue.h
	$(XCC) -S $(CFLAGS) kern.c
	mv kern.s ../bin/kern.s

../bin/%.s: %.c include/%.h
	$(XCC) -S $(CFLAGS) $<
	mv $*.s $@

../bin/kern.o: ../bin/kern.s syscall.s genAss.s
	$(AS) $(ASFLAGS) -o ../bin/kern.o ../bin/kern.s syscall.s genAss.s
	
../bin/%.o: ../bin/%.s
	$(AS) $(ASFLAGS) -o $@ $<
