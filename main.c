#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_AHBENR REG32(0x40021014u)
#define GPIOE_MODER REG32(0x48001000u)
#define GPIOE_ODR REG32(0x48001014u)

int main(void)
{
    // Enable GPIOE peripheral clock.
    RCC_AHBENR |= (1u << 21);

    // Configure PE9 as general-purpose output.
    GPIOE_MODER &= ~(3u << 18);
    GPIOE_MODER |= (1u << 18);

    for (;;)
    {
        GPIOE_ODR ^= (1u << 9);

        for (volatile uint32_t i = 0; i < 500000u; ++i)
        {
            __asm__ volatile("nop");
        }
    }
}
