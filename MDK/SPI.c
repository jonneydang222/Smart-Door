#include "SPI.h"

void SPI1_Init(void)
{
    // Enable clock GPIOA, GPIOB, SPI1
    RCC->APB2ENR |= (1U << 2) |
                    (1U << 3) |
                    (1U << 12);

    // PA4: GPIO Output Push-Pull
    // PA5: AF Push-Pull
    // PA6: Input Floating
    // PA7: AF Push-Pull
    GPIOA->CRL &= ~(
        (0xFU << 16) |
        (0xFU << 20) |
        (0xFU << 24) |
        (0xFU << 28)
    );

    GPIOA->CRL |= (
        (0x3U << 16) |
        (0xBU << 20) |
        (0x4U << 24) |
        (0xBU << 28)
    );

    // PB0: Output Push-Pull (RC522 RST)
    GPIOB->CRL &= ~(0xFU << 0);
    GPIOB->CRL |=  (0x3U << 0);

    // CS = HIGH, RST = HIGH
    GPIOA->ODR |= (1U << 4);
    GPIOB->ODR |= (1U << 0);

    // SPI1 configuration
    SPI1->CR1 = 0;
    SPI1->CR1 |= (1U << 2);  // Master
    SPI1->CR1 |= (3U << 3);  // PCLK/16
    SPI1->CR1 |= (1U << 8);  // SSI
    SPI1->CR1 |= (1U << 9);  // SSM
    SPI1->CR1 |= (1U << 6);  // Enable SPI
}

uint8_t SPI1_Transfer(uint8_t data)
{
    while(!(SPI1->SR & (1U << 1)));
    SPI1->DR = data;
    while(!(SPI1->SR & (1U << 0)));
    return (uint8_t)SPI1->DR;
}
