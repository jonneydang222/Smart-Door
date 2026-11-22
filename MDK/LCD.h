#ifndef __LCD_H
#define __LCD_H

#include "stm32f10x.h"

void Delay_ms(uint32_t ms);
void LCD_Init(void);
void LCD_Send4Bit(unsigned char data, unsigned char rs);
void LCD_Command(unsigned char cmd);
void LCD_Data(unsigned char data);
void LCD_CreateChar(unsigned char location);
void LCD_PrintString(uint8_t row, uint8_t col, const char *str);
void LCD_ClearRow(uint8_t row);

#endif
