
#include "stm32f10x.h"
#include "SPI.h"
#include "RC522.h"
#include "Servo.h"
#include "I2C.h"
#include "LCD.h"
#include "Buzzer.h"

#define MAX_FAIL_COUNT  3
#define LOCKOUT_SECONDS 10

// UID the duoc phep mo cua
static const uint8_t AUTHORIZED_UID[4] =
{
    0x92, 0x9C, 0x6D, 0xCF
};

volatile uint8_t Version = 0;
volatile uint8_t TestResult = 0;
volatile uint8_t AnticollResult = 0;

volatile uint8_t TestData[2] = {0, 0};
volatile uint8_t UID[5] = {0, 0, 0, 0, 0};

volatile uint8_t failCount = 0;

// ================= BUZZER =================

static void Tone_Success(void)
{
    Buzzer_On();
    Delay_ms(120);
    Buzzer_Off();

    Delay_ms(80);

    Buzzer_On();
    Delay_ms(220);
    Buzzer_Off();
}

static void Tone_Error(void)
{
    Buzzer_On();
    Delay_ms(450);
    Buzzer_Off();
}

static void Tone_Locked(void)
{
    uint8_t k;

    for(k = 0; k < 3; k++)
    {
        Buzzer_On();
        Delay_ms(150);

        Buzzer_Off();
        Delay_ms(120);
    }
}

// ================= LCD =================

static char HexNibble(uint8_t v)
{
    v &= 0x0F;

    if(v < 10)
        return '0' + v;

    return 'A' + (v - 10);
}

static void Update_Diag_On_LCD(void)
{
    char line[21];

    line[0] = 'V';
    line[1] = ':';
    line[2] = HexNibble(Version >> 4);
    line[3] = HexNibble(Version);

    line[4] = ' ';
    line[5] = 'R';
    line[6] = ':';
    line[7] = '0' + (TestResult & 1);

    line[8] = ' ';
    line[9] = 'A';
    line[10] = ':';
    line[11] = '0' + (AnticollResult & 1);

    line[12] = ' ';
    line[13] = 'I';
    line[14] = ':';
    line[15] = HexNibble(DebugIRQ >> 4);
    line[16] = HexNibble(DebugIRQ);

    line[17] = 'E';
    line[18] = ':';
    line[19] = HexNibble(DebugError);

    line[20] = '\0';

    LCD_PrintString(3, 0, line);
}

static void Show_ReadyScreen(void)
{
    LCD_Command(0x01);
    Delay_ms(2);

    LCD_PrintString(0, 0, "SMART DOOR LOCK");
    LCD_PrintString(1, 0, "Trang thai: DONG");
    LCD_PrintString(2, 0, "Moi quet the RFID");

    Update_Diag_On_LCD();
}

// ================= RFID =================

static uint8_t IsAuthorizedCard(const uint8_t *uid)
{
    uint8_t i;

    for(i = 0; i < 4; i++)
    {
        if(uid[i] != AUTHORIZED_UID[i])
            return 0;
    }

    return 1;
}

// ================= LOCKOUT =================

static void Handle_LockedState(void)
{
    uint8_t secLeft;
    char buf[21];

    Tone_Locked();

    LCD_Command(0x01);
    Delay_ms(2);

    LCD_PrintString(0, 0, "!!! CANH BAO !!!");
    LCD_PrintString(1, 0, "HE THONG DA KHOA");
    LCD_PrintString(2, 0, "Sai the >= 3 lan");

    for(secLeft = LOCKOUT_SECONDS;
        secLeft > 0;
        secLeft--)
    {
        buf[0] = 'M';
        buf[1] = 'o';
        buf[2] = ' ';
        buf[3] = 's';
        buf[4] = 'a';
        buf[5] = 'u';
        buf[6] = ':';
        buf[7] = ' ';

        if(secLeft >= 10)
        {
            buf[8] = '0' + secLeft / 10;
            buf[9] = '0' + secLeft % 10;
            buf[10] = 's';
            buf[11] = '\0';
        }
        else
        {
            buf[8] = '0' + secLeft;
            buf[9] = 's';
            buf[10] = '\0';
        }

        LCD_ClearRow(3);
        LCD_PrintString(3, 0, buf);

        Delay_ms(1000);
    }

    failCount = 0;

    Show_ReadyScreen();
}

// ================= MAIN =================

int main(void)
{
    uint8_t uidTemp[5] = {0, 0, 0, 0, 0};
    uint8_t i;

    uint8_t prevV = 0xFF;
    uint8_t prevR = 0xFF;
    uint8_t prevA = 0xFF;
    uint8_t prevI = 0xFF;
    uint8_t prevE = 0xFF;

    // Khoi tao cac module
    Servo_Init();
    I2C1_Init();
    LCD_Init();
    Buzzer_Init();

    SPI1_Init();
    RC522_Init();

    // Trang thai ban dau
    Servo_Close();
    Buzzer_Off();

    Delay_ms(50);

    Version = RC522_ReadRegister(0x37);

    Show_ReadyScreen();

    while(1)
    {
        Version = RC522_ReadRegister(0x37);

        // Kiem tra ket noi RC522
        if(Version == 0x00 || Version == 0xFF)
        {
            TestResult = 0;
            AnticollResult = 0;

            Delay_ms(200);

            RC522_Init();

            continue;
        }

        // Doc the RFID
        TestResult = RC522_Wakeup((uint8_t *)TestData);

        if(TestResult == 1)
				{
						AnticollResult = RC522_Anticoll(uidTemp);

						if(AnticollResult == 1)
						{
								if(!RC522_Select(uidTemp))
								{
										AnticollResult = 0;
								}
						}
				}
				else
				{
						AnticollResult = 0;
				}

        // Cap nhat thong tin debug tren LCD
        if(Version != prevV ||
           TestResult != prevR ||
           AnticollResult != prevA ||
           DebugIRQ != prevI ||
           DebugError != prevE)
        {
            Update_Diag_On_LCD();

            prevV = Version;
            prevR = TestResult;
            prevA = AnticollResult;
            prevI = DebugIRQ;
            prevE = DebugError;
        }

        // Da doc duoc the
        if(TestResult == 1 && AnticollResult == 1)
        {
            for(i = 0; i < 5; i++)
            {
                UID[i] = uidTemp[i];
            }

            // The hop le
            if(IsAuthorizedCard(uidTemp))
            {
                failCount = 0;

                LCD_Command(0x01);
                Delay_ms(2);

                LCD_PrintString(0, 0, "THE HOP LE!");
                LCD_PrintString(1, 0, "Trang thai: MO CUA");
                LCD_PrintString(2, 0, "Servo: 90 do");

                Tone_Success();

                Servo_Open();

                Delay_ms(3000);

                Servo_Close();

                LCD_PrintString(1, 0, "Trang thai: DONG ");
                LCD_PrintString(2, 0, "Servo: 0 do      ");

                Delay_ms(1000);

                prevV = 0xFF;

                Show_ReadyScreen();
            }

            // The khong hop le
            else
            {
                failCount++;

                if(failCount >= MAX_FAIL_COUNT)
                {
                    Handle_LockedState();

                    prevV = 0xFF;
                }
                else
                {
                    char errLine[21];

                    Tone_Error();

                    LCD_Command(0x01);
                    Delay_ms(2);

                    LCD_PrintString(0, 0, "THE KHONG HOP LE!");
                    LCD_PrintString(1, 0, "Trang thai: DONG");

                    errLine[0] = 'S';
                    errLine[1] = 'a';
                    errLine[2] = 'i';
                    errLine[3] = ' ';
                    errLine[4] = 'l';
                    errLine[5] = 'a';
                    errLine[6] = 'n';
                    errLine[7] = ' ';

                    errLine[8] = '0' + failCount;
                    errLine[9] = '/';
                    errLine[10] = '0' + MAX_FAIL_COUNT;
                    errLine[11] = '\0';

                    LCD_PrintString(2, 0, errLine);
                    LCD_PrintString(3, 0, "Thu lai...");

                    Delay_ms(1000);
										RC522_Halt();

                    prevV = 0xFF;
                    Show_ReadyScreen();
                }
            }

            Delay_ms(600);
        }
    }
}
