#include "RC522.h"
#include "SPI.h"

#define PICC_ANTICOLL 0x93

#define PICC_SELECT  0x93
#define PICC_HALT    0x50
#define PICC_WUPA    0x52
// RC522 pins
#define RC522_CS_LOW()   (GPIOA->ODR &= ~(1U << 4))
#define RC522_CS_HIGH()  (GPIOA->ODR |=  (1U << 4))

#define RC522_RST_LOW()  (GPIOB->ODR &= ~(1U << 0))
#define RC522_RST_HIGH() (GPIOB->ODR |=  (1U << 0))

// Registers
#define CommandReg       0x01
#define ComIrqReg        0x04
#define ErrorReg         0x06
#define FIFODataReg      0x09
#define FIFOLevelReg     0x0A
#define ControlReg       0x0C
#define BitFramingReg    0x0D

#define TModeReg         0x2A
#define TPrescalerReg    0x2B
#define TReloadRegH      0x2C
#define TReloadRegL      0x2D
#define TxASKReg         0x15
#define ModeReg          0x11
#define TxControlReg     0x14

// Commands
#define PCD_IDLE         0x00
#define PCD_TRANSCEIVE   0x0C
#define PCD_SOFTRESET    0x0F

// Debug variables
// Debug variables
volatile uint8_t DebugVersion = 0;
volatile uint8_t DebugIRQ = 0;
volatile uint8_t DebugError = 0;
volatile uint8_t DebugFIFO = 0;
volatile uint8_t DebugATQA[2] = {0, 0};

volatile uint8_t RxCountSnap = 0;
volatile uint8_t RxBitsSnap = 0;

static void Delay(void)
{
    volatile uint32_t i;
    for(i = 0; i < 100000; i++);
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp | mask);
}

static void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp & (~mask));
}

static void RC522_CalculateCRC(
    uint8_t *data,
    uint8_t length,
    uint8_t *result)
{
    uint8_t i;
    uint8_t irq;
    uint16_t timeout;

    RC522_WriteRegister(CommandReg, PCD_IDLE);
    RC522_WriteRegister(ComIrqReg, 0x04);
    RC522_WriteRegister(FIFOLevelReg, 0x80);

    for(i = 0; i < length; i++)
    {
        RC522_WriteRegister(FIFODataReg, data[i]);
    }

    RC522_WriteRegister(CommandReg, 0x03);

    timeout = 0;

    do
    {
        irq = RC522_ReadRegister(ComIrqReg);
        timeout++;
    }
    while(timeout < 30000 && !(irq & 0x04));

    RC522_WriteRegister(CommandReg, PCD_IDLE);

    if(timeout >= 30000)
    {
        result[0] = 0;
        result[1] = 0;
        return;
    }

    result[0] = RC522_ReadRegister(0x22);
    result[1] = RC522_ReadRegister(0x21);
}

void RC522_WriteRegister(uint8_t address, uint8_t value)
{
    RC522_CS_LOW();
    SPI1_Transfer((address << 1) & 0x7E);
    SPI1_Transfer(value);
    while(SPI1->SR & (1U << 7));
    RC522_CS_HIGH();
}

uint8_t RC522_ReadRegister(uint8_t address)
{
    uint8_t value;
    RC522_CS_LOW();
    SPI1_Transfer((address << 1) | 0x80);
    value = SPI1_Transfer(0x00);
    while(SPI1->SR & (1U << 7));
    RC522_CS_HIGH();
    return value;
}

static uint8_t RC522_Transceive(
    uint8_t *sendData,
    uint8_t sendLen,
    uint8_t *backData,
    uint8_t *backLen,
    uint8_t *lastBits)
{
    uint8_t i;
    uint8_t irq;
    uint8_t error;
    uint8_t capacity;
    uint16_t timeout;

    capacity = *backLen;
    *backLen = 0;
    *lastBits = 0;

    RC522_WriteRegister(CommandReg, PCD_IDLE);
    RC522_WriteRegister(ComIrqReg, 0x7F);
    RC522_WriteRegister(FIFOLevelReg, 0x80);

    for(i = 0; i < sendLen; i++)
    {
        RC522_WriteRegister(FIFODataReg, sendData[i]);
    }

    RC522_WriteRegister(CommandReg, PCD_TRANSCEIVE);
    RC522_SetBitMask(BitFramingReg, 0x80);

    timeout = 0;
    do
    {
        irq = RC522_ReadRegister(ComIrqReg);
        timeout++;
    }
    while(
        timeout < 30000 &&
        !(irq & 0x30) &&
        !(irq & 0x01)
    );

    RC522_ClearBitMask(BitFramingReg, 0x80);

    DebugIRQ = irq;
    error = RC522_ReadRegister(ErrorReg);
    DebugError = error;
    RxCountSnap = RC522_ReadRegister(FIFOLevelReg) & 0x7F;
    RxBitsSnap = RC522_ReadRegister(ControlReg) & 0x07;

    if(timeout >= 30000)
        return 0;

    if(irq & 0x01)
        return 0;

    if(!(irq & 0x20))
        return 0;

    if(error & 0x1B)
        return 0;

    if(RxCountSnap == 0 || RxCountSnap > capacity)
        return 0;

    *backLen = RxCountSnap;
    *lastBits = RxBitsSnap;

    for(i = 0; i < *backLen; i++)
    {
        backData[i] = RC522_ReadRegister(FIFODataReg);
    }

    return 1;
}

void RC522_Init(void)
{
    RC522_RST_LOW();
    Delay();
    RC522_RST_HIGH();
    Delay();

    RC522_WriteRegister(CommandReg, PCD_SOFTRESET);
    Delay();

    RC522_WriteRegister(TModeReg, 0x8D);
    RC522_WriteRegister(TPrescalerReg, 0x3E);
    RC522_WriteRegister(TReloadRegH, 0x03);
    RC522_WriteRegister(TReloadRegL, 0xE8);
    RC522_WriteRegister(TxASKReg, 0x40);
    RC522_WriteRegister(ModeReg, 0x3D);
    RC522_SetBitMask(TxControlReg, 0x03);
}

uint8_t RC522_TestREQA(uint8_t *result)
{
    uint8_t command;
    uint8_t data[2];
    uint8_t length;
    uint8_t lastBits;

    command = 0x26;
    data[0] = 0;
    data[1] = 0;
    length = 2;
    lastBits = 0;

    RC522_WriteRegister(BitFramingReg, 0x07);

    if(
        !RC522_Transceive(
            &command,
            1,
            data,
            &length,
            &lastBits
        )
        || length != 2
        || lastBits != 0
    )
    {
        result[0] = 0;
        result[1] = 0;
        return 0;
    }

    result[0] = data[0];
    result[1] = data[1];
    return 1;
}

uint8_t RC522_Anticoll(uint8_t *uid)
{
    uint8_t sendData[2];
    uint8_t length;
    uint8_t lastBits;
    uint8_t i;
    uint8_t bcc;

    sendData[0] = PICC_ANTICOLL;
    sendData[1] = 0x20;

    length = 5;
    lastBits = 0;

    RC522_WriteRegister(BitFramingReg, 0x00);

    if(!RC522_Transceive(
        sendData,
        2,
        uid,
        &length,
        &lastBits))
    {
        return 0;
    }

    if(length != 5 || lastBits != 0)
    {
        return 0;
    }

    bcc = 0;
    for(i = 0; i < 4; i++)
    {
        bcc ^= uid[i];
    }

    if(bcc != uid[4])
    {
        return 0;
    }

    return 1;
}

uint8_t RC522_Select(uint8_t *uid)
{
    uint8_t buffer[9];
    uint8_t response[3];
    uint8_t length;
    uint8_t lastBits;

    buffer[0] = PICC_SELECT;
    buffer[1] = 0x70;

    buffer[2] = uid[0];
    buffer[3] = uid[1];
    buffer[4] = uid[2];
    buffer[5] = uid[3];
    buffer[6] = uid[4];

    RC522_CalculateCRC(buffer, 7, &buffer[7]);

    length = 3;
    lastBits = 0;

    if(!RC522_Transceive(
        buffer,
        9,
        response,
        &length,
        &lastBits))
    {
        return 0;
    }

    return (length == 3 && lastBits == 0);
}

void RC522_Halt(void)
{
    uint8_t buffer[4];
    uint8_t response[4];
    uint8_t length;
    uint8_t lastBits;

    buffer[0] = PICC_HALT;
    buffer[1] = 0x00;

    RC522_CalculateCRC(buffer, 2, &buffer[2]);

    length = sizeof(response);
    lastBits = 0;

    RC522_Transceive(
        buffer,
        4,
        response,
        &length,
        &lastBits);
}

uint8_t RC522_Wakeup(uint8_t *result)
{
    uint8_t command;
    uint8_t data[2];
    uint8_t length;
    uint8_t lastBits;

    command = PICC_WUPA;
    length = 2;
    lastBits = 0;

    RC522_WriteRegister(BitFramingReg, 0x07);

    if(!RC522_Transceive(
        &command,
        1,
        data,
        &length,
        &lastBits))
    {
        return 0;
    }

    if(length != 2 || lastBits != 0)
        return 0;

    result[0] = data[0];
    result[1] = data[1];

    return 1;
}