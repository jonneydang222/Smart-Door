#include "I2C.h"

void I2C1_Init(void)
{
    RCC->APB2ENR |= (1 << 3);
    RCC->APB1ENR |= (1 << 21);

    // PB6, PB7: Alternate Function Open Drain
    GPIOB->CRL &= ~(0xFF << 24);
    GPIOB->CRL |=  (0xFF << 24);

    I2C1->CR1 = 0;
    I2C1->CR2 = 36;
    I2C1->CCR = 180;
    I2C1->TRISE = 37;

    I2C1->CR1 |= (1 << 0);
}

void I2C1_Start(void)
{
    I2C1->CR1 |= (1 << 8);
    while(!(I2C1->SR1 & (1 << 0)));
    (void)I2C1->SR1;
    (void)I2C1->SR2;
}

void I2C1_Stop(void)
{
    I2C1->CR1 |= (1 << 9);
}

void I2C1_Write(unsigned char data)
{
    while(!(I2C1->SR1 & (1 << 7)));
    I2C1->DR = data;
    while(!(I2C1->SR1 & (1 << 2)));
}

void PCF8574_Write(unsigned char data)
{
    I2C1_Start();
    I2C1->DR = (LCD_ADDR << 1);
    while(!(I2C1->SR1 & (1 << 1)));
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    I2C1_Write(data);
    I2C1_Stop();
}
