# Getting started with STM32F303 Discovery LED blinking

Board: STM32F3DISCOVERY

MCU: STM32F303VCT6

CPU: ARM Cortex-M4F, up to 72 MHz

Memory: 256 KB Flash, 48 KB SRAM

Debugger: Onboard ST-LINK

We'll use a native Linux development workflow:

- GCC ARM Embedded — C/C++ compiler.
- OpenOCD — flashing and debugging.
- GDB — source-level debugging.

The first goal is to compile a small firmware program, flash it over USB, and blink an onboard LED.

## 1. Install development tools


```sh
sudo dnf install -y \
  arm-none-eabi-gcc \
  openocd \
  gdb \
  usbutils
```

Verify installation:

```sh
which arm-none-eabi-gcc
arm-none-eabi-gcc --version
openocd --version
gdb --version
```



## 2. Connect your Discovery board

1. Ensure the JP3 jumper is installed.
2. Ensure both CN4 jumpers are installed for onboard debugging.
3. Connect a data-capable USB Mini-B cable to USB ST-LINK, not USB USER.
4. Connect the cable to your Fedora computer.

The power LED (LD1) should illuminate. The factory demo firmware may also start running.

Check USB enumeration:

```sh
lsusb | grep -i -E 'stmicro|st-link'
```

For more information:

```sh
lsusb
sudo dmesg | tail -30
```

STMicroelectronics' USB vendor ID is commonly `0483`.

## 3. Test the debugger

First, check that OpenOCD includes the board configuration:

```sh
find /usr/share/openocd -iname '*stm32f3discovery*'
```

Then launch:

```sh
openocd -f board/stm32f3discovery.cfg
```

If that configuration is unavailable, try:

```sh
openocd -f interface/stlink.cfg -f target/stm32f3x.cfg
```

A successful connection should report the STM32 target and start a GDB server on TCP port 3333.

Keep OpenOCD running in this terminal. Use Ctrl+C to stop it.

If you get a USB permission error, check the installed udev rules:

```sh
rpm -ql openocd | grep -E 'udev|rules'
```

As a temporary diagnostic only, retry OpenOCD with `sudo`. For regular development, configure appropriate udev permissions rather than running the debugger as root.

## 4. Compile and flash your first firmware

For the first experiment, use direct register access without HAL or an RTOS.

The example below blinks LD3 (red), connected to PE9, using the MCU's default internal clock.


The repository includes the three source files below. To follow along in a separate directory, create a project and the files shown:

```sh
mkdir -p ~/stm32/blink
cd ~/stm32/blink
```

Create `main.c`:

```c

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_AHBENR   REG32(0x40021014u)
#define GPIOE_MODER  REG32(0x48001000u)
#define GPIOE_ODR    REG32(0x48001014u)

int main(void) {
    // Enable GPIOE peripheral clock.
    RCC_AHBENR |= (1u << 21);

    // Configure PE9 as general-purpose output.
    GPIOE_MODER &= ~(3u << 18);
    GPIOE_MODER |=  (1u << 18);

    for (;;) {
        GPIOE_ODR ^= (1u << 9);

        for (volatile uint32_t i = 0; i < 500000u; ++i) {
            __asm__ volatile ("nop");
        }
    }
}

```

Create `startup.c`:

```c

#include <stdint.h>

extern int main(void);
extern uint32_t _estack;

void Reset_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector"), used))
const uintptr_t vectors[] = {
    (uintptr_t)&_estack,
    (uintptr_t)Reset_Handler
};

void Reset_Handler(void) {
    main();
    for (;;) {}
}

void Default_Handler(void) {
    for (;;) {}
}

```

Create `linker.ld`:

```ld

ENTRY(Reset_Handler)

MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 40K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .isr_vector : {
        KEEP(*(.isr_vector))
    } > FLASH

    .text : {
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data : {
        *(.data*)
    } > RAM AT > FLASH

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    } > RAM
}

```

This minimal linker script reserves the main 40 KB SRAM region. The remaining 8 KB is separate core-coupled memory (CCM). The startup code is sufficient for this register-only demonstration, but not yet for general C programs using initialized globals, interrupts, or the standard library.

Compile:

```sh
arm-none-eabi-gcc \
  -mcpu=cortex-m4 \
  -mthumb \
  -O2 -g \
  -ffreestanding \
  -nostdlib \
  -Wl,--gc-sections \
  -T linker.ld \
  startup.c main.c \
  -o blink.elf
```

Flash with OpenOCD (stop the earlier OpenOCD session first):

```sh
openocd -f board/stm32f3discovery.cfg -c "program blink.elf verify reset exit"
```

The red LD3 LED should blink. This replaces the factory demo firmware.

## 5. Debug with GDB

Start OpenOCD in one terminal:

```sh
openocd -f board/stm32f3discovery.cfg
```

In another terminal:

```sh
gdb blink.elf
```

Inside GDB:

```sh
set architecture arm
target extended-remote localhost:3333
monitor reset halt
break main
continue
```

You can now inspect registers, set breakpoints, and step through firmware.

