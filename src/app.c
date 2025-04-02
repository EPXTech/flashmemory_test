#include "app.h"
#include "definitions.h"

/* Callbacks */
void toggle_user_led(){
    GPIO_PC18_Toggle();
}

// Example function to send a JEDEC ID read command via QSPI in Instruction Mode.

void sendJEDECRead(void)
{
    uint8_t jedecId[3];
    uint32_t rxWord = 0;

    // Set up the JEDEC ID read transfer (0x9F command, no dummy cycles, single-bit SPI)
    qspi_register_xfer_t readJEDECId =
    {
        .instruction = 0x9F,              // JEDEC ID Read opcode
        .width = SINGLE_BIT_SPI,          // Standard SPI mode
        .dummy_cycles = 0                 // No dummy cycles required for JEDEC ID
    };

    // Perform the transfer (read 3 bytes into a 32-bit buffer)
    bool success = QSPI_RegisterRead(&readJEDECId, &rxWord, 3);
    if (!success)
    {
        SERCOM2_USART_Write("[APP] QSPI JEDEC ID Read failed.\r\n", strlen("[APP] QSPI JEDEC ID Read failed.\r\n"));
        return;
    }

    // Extract the 3 bytes from the 32-bit word
    jedecId[0] = (rxWord >> 0) & 0xFF;
    jedecId[1] = (rxWord >> 8) & 0xFF;
    jedecId[2] = (rxWord >> 16) & 0xFF;

    // Format and print the result
    char logBuffer[64];
    sprintf(logBuffer, "[APP] JEDEC ID: %02X %02X %02X\r\n", jedecId[0], jedecId[1], jedecId[2]);
    SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
    while (SERCOM2_USART_WriteIsBusy());
}

APP_DATA appData;

void APP_Initialize(void)
{
    // Initialize application state
    appData.state = APP_STATE_INIT;
    
    TC0_TimerCallbackRegister(toggle_user_led, 0);
    TC0_TimerStart();
}

void APP_Tasks(void)
{    
    switch (appData.state)
    {
        case APP_STATE_INIT:
        {
            appData.state = APP_STATE_RUNNING;
            break;
        }
        case APP_STATE_RUNNING:
        {
            sendJEDECRead();
            break;
        }
        case APP_STATE_ERROR:
        {
            break;
        }
        default:
        {
            break;
        }
    }  
}
