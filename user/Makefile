XCC = gcc
AS = as
CFLAGS = -c -O2 -fPIC -Wall -I../lib/include -Iinclude -mcpu=arm920t -msoft-float

ASFLAGS = -mcpu=arm920t -mapcs-32

all: ../bin/userTasks.o ../bin/nameServer.o ../bin/clockServer.o

../bin/%.s: %.c include/%.h
	$(XCC) -S $(CFLAGS) $<
	mv $*.s $@

../bin/%.o: ../bin/%.s
	$(AS) $(ASFLAGS) -o $@ $<
