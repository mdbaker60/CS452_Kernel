XCC = gcc
AS = as

ASFLAGS = -mcpu=arm920t -mapcs-32

all: ../bin/string.o ../bin/prng.o ../bin/memcpy.o

../bin/memcpy.o: memcpy.s
	$(AS) $(ASFLAGS) -o ../bin/memcpy.o memcpy.s

../bin/%.s: %.c include/%.h
	$(XCC) -S $(CFLAGS) $<
	mv $*.s $@

../bin/%.o: ../bin/%.s
	$(AS) $(ASFLAGS) -o $@ $<
