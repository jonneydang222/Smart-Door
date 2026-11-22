#ifndef __RC522_H
#define __RC522_H

#include "stm32f10x.h"

extern volatile uint8_t DebugVersion;
extern volatile uint8_t DebugIRQ;
extern volatile uint8_t DebugError;
extern volatile uint8_t DebugFIFO;
extern volatile uint8_t DebugATQA[2];

void RC522_Init(void);

uint8_t RC522_ReadRegister(uint8_t address);
void RC522_WriteRegister(uint8_t address, uint8_t value);

uint8_t RC522_TestREQA(uint8_t *result);
uint8_t RC522_Anticoll(uint8_t *uid);
uint8_t RC522_ReadUID(uint8_t *uid);

uint8_t RC522_Select(uint8_t *uid);
void RC522_Halt(void);
uint8_t RC522_Wakeup(uint8_t *result);

#endif
