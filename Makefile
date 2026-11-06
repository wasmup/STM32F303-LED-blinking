CC = arm-none-eabi-gcc
OPENOCD ?= openocd

CFLAGS ?= -mcpu=cortex-m4 -mthumb -O2 -g -ffreestanding
LDFLAGS ?= -nostdlib -Wl,--gc-sections -T linker.ld

.PHONY: all build run flash clean

all:
	$(MAKE) clean
	$(MAKE) run

build: blink.elf

blink.elf: startup.c main.c linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) startup.c main.c -o $@

run flash: blink.elf
	$(OPENOCD) -f stm32f3.cfg -c "program blink.elf verify reset exit"

clean:
	$(RM) blink.elf
