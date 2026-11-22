#include "LCD.h"
#include "I2C.h"

#define LCD_RS    0x01
#define LCD_EN    0x04
#define LCD_BL    0x08

void Delay_ms(uint32_t ms)
{
    uint32_t i;

    SysTick->LOAD = 72000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 5;

    for(i = 0; i < ms; i++)
    {
        while((SysTick->CTRL & (1 << 16)) == 0);
    }

    SysTick->CTRL = 0;
}

void LCD_Send4Bit(unsigned char data, unsigned char rs)
{
    unsigned char value;

    value = (data & 0xF0) | LCD_BL | rs;

    PCF8574_Write(value | LCD_EN);
    Delay_ms(1);

    PCF8574_Write(value);
    Delay_ms(1);
}

void LCD_Command(unsigned char cmd)
{
    LCD_Send4Bit(cmd & 0xF0, 0);
    LCD_Send4Bit(cmd << 4, 0);
    Delay_ms(2);
}

void LCD_Data(unsigned char data)
{
    LCD_Send4Bit(data & 0xF0, LCD_RS);
    LCD_Send4Bit(data << 4, LCD_RS);
    Delay_ms(1);
}

void LCD_Init(void)
{
    Delay_ms(50);

    LCD_Send4Bit(0x30, 0);
    Delay_ms(5);

    LCD_Send4Bit(0x30, 0);
    Delay_ms(5);

    LCD_Send4Bit(0x30, 0);
    Delay_ms(5);

    LCD_Send4Bit(0x20, 0);
    Delay_ms(5);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);

    Delay_ms(5);
}

void LCD_CreateChar(unsigned char location)
{
    unsigned char pattern[8] = {
        0x08,
        0x04,
        0x0E,
        0x01,
        0x0F,
        0x11,
        0x0F,
        0x00
    };

    unsigned char i;

    LCD_Command(0x40 | (location << 3));

    for(i = 0; i < 8; i++)
    {
        LCD_Data(pattern[i]);
    }
}

void LCD_PrintString(uint8_t row, uint8_t col, const char *str)
{
    static const uint8_t rowAddr[4] = {0x80, 0xC0, 0x94, 0xD4};

    if(row > 3 || col > 19)
        return;

    LCD_Command(rowAddr[row] + col);

    while(*str)
    {
        LCD_Data((unsigned char)(*str));
        str++;
    }
}

void LCD_ClearRow(uint8_t row)
{
    uint8_t i;
    if(row > 3)
        return;

    LCD_PrintString(row, 0, "                    ");
    (void)i;
}
