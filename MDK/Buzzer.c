#include "stm32f10x.h"
#include "Buzzer.h"
#include "Delay.h"

void Buzzer_Init(void)
{
    RCC->APB2ENR |= (1U << 3);

    // PB1: Output Push-Pull
    GPIOB->CRL &= ~(0xFU << 4);
    GPIOB->CRL |=  (0x3U << 4);

    // Tat buzzer
    GPIOB->BRR = (1U << 1);
}

void Buzzer_On(void)
{
    GPIOB->BSRR = (1U << 1);
}

void Buzzer_Off(void)
{
    GPIOB->BRR = (1U << 1);
}

void Buzzer_Beep(void)
{
    Buzzer_On();
    Delay(200);
    Buzzer_Off();
}
