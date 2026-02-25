ROMNAME = fukka

all:
	$(PVSNESLIB_HOME)/devkitsnes/bin/816-tcc \
	-I$(PVSNESLIB_HOME)/include \
	-O2 \
	-o $(ROMNAME).elf \
	main.c \
	-L$(PVSNESLIB_HOME)/lib \
	-lpvsneslib

	$(PVSNESLIB_HOME)/devkitsnes/bin/816-objcopy \
	-O binary \
	$(ROMNAME).elf \
	$(ROMNAME).sfc
