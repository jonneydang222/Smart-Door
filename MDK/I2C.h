#ifndef __I2C_H
#define __I2C_H

#include "stm32f10x.h"

#define LCD_ADDR 0x27

void I2C1_Init(void);
void I2C1_Start(void);
void I2C1_Stop(void);
void I2C1_Write(unsigned char data);
void PCF8574_Write(unsigned char data);

#endif
