#include "Servo.h"

void Servo_Init(void)
{
    RCC->APB2ENR |= (1 << 2);
    RCC->APB1ENR |= (1 << 0);

    // PA0: Alternate Function Push Pull
    GPIOA->CRL &= ~(0xF << 0);
    GPIOA->CRL |= (0xB << 0);

    // Timer 2: 1 MHz
    TIM2->PSC = 71;

    // Chu ky 20 ms
    TIM2->ARR = 19999;

    // PWM Mode 1
    TIM2->CCMR1 &= ~(7 << 4);
    TIM2->CCMR1 |= (6 << 4);
    TIM2->CCMR1 |= (1 << 3);

    // Enable channel 1
    TIM2->CCER |= (1 << 0);
    TIM2->CR1 |= (1 << 7);

    // Vi tri dong cua ban dau: 0 do
    TIM2->CCR1 = 500;

    TIM2->EGR |= (1 << 0);
    TIM2->CR1 |= (1 << 0);
}

void Servo_SetAngle(uint8_t angle)
{
    if(angle > 180)
        angle = 180;

    TIM2->CCR1 = 500 + ((uint32_t)angle * 2000) / 180;
}

void Servo_Open(void)
{
    Servo_SetAngle(90);
}

void Servo_Close(void)
{
    Servo_SetAngle(0);
}
