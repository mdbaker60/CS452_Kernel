XCC = gcc
AS = as

ASFLAGS = -mcpu=arm920t -mapcs-32

all: CFLAGS += -I../lib/include
all: ../bin/userTasks.o ../bin/nameServer.o ../bin/clockServer.o ../bin/systemTasks.o ../bin/io.o ../bin/term.o

../bin/%.s: %.c include/%.h
	$(XCC) -S $(CFLAGS) $<
	mv $*.s $@

../bin/%.o: ../bin/%.s
	$(AS) $(ASFLAGS) -o $@ $<
