LD = ld

LDFLAGS = -init main -Map bin/kern.map -N -T kern/orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L/u/wbcowan/cs452/io/lib

all: bin/kern_a3.elf

recursive:
	cd kern; make
	cd user; make
	cd lib; make

bin/kern_a3.elf: recursive $(wildcard bin/*.o)
	$(LD) $(LDFLAGS) -o bin/kern_a3.elf bin/*.o -lbwio -lgcc
	cp bin/kern_a3.elf /u/cs452/tftp/ARM/djgroot/kern_a3.elf
