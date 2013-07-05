XCC = gcc
AS = as

ASFLAGS = -mcpu=arm920t -mapcs-32

all: CFLAGS += -I../lib/include
all: ../bin/userTasks.o ../bin/nameServer.o ../bin/clockServer.o ../bin/systemTasks.o ../bin/io.o ../bin/term.o ../bin/train.o ../bin/track.o ../bin/velocity.o

../bin/track.s: track.c include/track.h include/track_node.h
	$(XCC) -S $(CFLAGS) track.c
	mv track.s ../bin/track.s

../bin/%.s: %.c include/%.h
	$(XCC) -S $(CFLAGS) $<
	mv $*.s $@

../bin/%.o: ../bin/%.s
	$(AS) $(ASFLAGS) -o $@ $<
