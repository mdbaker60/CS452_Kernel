XCC = gcc
AS = as

ASFLAGS = -mcpu=arm920t -mapcs-32

all: CFLAGS += -I../lib/include -I../user/include
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
