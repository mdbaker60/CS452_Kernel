.SUFFIXED:
MAKEFLAGS += -r

export CFLAGS = -c -O2 -fPIC -Wall -mcpu=arm920t -msoft-float -Iinclude

LD = ld

LDFLAGS = -init main -Map bin/kern.map -N -T kern/orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L/u/wbcowan/cs452/io/lib

all: bin/kern_p.elf

.PHONY: recursive
recursive:
	cd kern && $(MAKE)
	cd user && $(MAKE)
	cd lib && $(MAKE)

bin/kern_p.elf: recursive $(wildcard bin/*.o)
	$(LD) $(LDFLAGS) -o bin/kern_p.elf bin/*.o -lbwio -lgcc
	cp bin/kern_p.elf /u/cs452/tftp/ARM/djgroot/kern_p.elf

bin/kern.o:
	@:

.PHONY: debug
debug: CFLAGS += -DDEBUG
debug: bin/kern_p.elf
