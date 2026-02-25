
# PVSnesLib project (devkitPro)
# Expected environment:
#   export PVSNESLIB=/path/to/pvsneslib
# Build:
#   make

ROMNAME = fukka_snes

CC      = 816-tcc
CFLAGS  = -I$(PVSNESLIB)/include -O2
LDFLAGS = -L$(PVSNESLIB)/lib -lpvsneslib

all:
	$(CC) $(CFLAGS) -o $(ROMNAME).elf main.c $(LDFLAGS)
	816-tcc -o $(ROMNAME).sfc $(ROMNAME).elf

clean:
	rm -f *.elf *.sfc
