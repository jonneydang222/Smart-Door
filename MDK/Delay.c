#include "Delay.h"

void Delay(uint32_t ms)
{
    uint32_t i;

    for(i = 0; i < ms; i++)
    {
        SysTick->LOAD = 72000 - 1;
        SysTick->VAL = 0;
        SysTick->CTRL = 5;

        while((SysTick->CTRL & (1U << 16)) == 0);

        SysTick->CTRL = 0;
    }
}
