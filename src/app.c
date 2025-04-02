#include "app.h"
#include "definitions.h"

/* Callbacks */
void toggle_user_led(){
    GPIO_PC18_Toggle();
}

// Example function to send a JEDEC ID read command via QSPI in Instruction Mode.
void sendJEDECRead(void)
{
    // Set the instruction address to 0 if not used by the command.
    QSPI_REGS->QSPI_INSTRADDR = 0x00000000;

    // Configure the instruction control register:
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(0x9F); // Opcode for JEDEC read

    // Configure the instruction frame register:
    QSPI_REGS->QSPI_INSTRFRAME = QSPI_INSTRFRAME_WIDTH_SINGLE_BIT_SPI |
                                 QSPI_INSTRFRAME_INSTREN(1) |
                                 QSPI_INSTRFRAME_DATAEN(1) |
                                 QSPI_INSTRFRAME_ADDRLEN(0) | 
                                 QSPI_INSTRFRAME_OPTCODEEN(0) |
                                 QSPI_INSTRFRAME_OPTCODELEN(0) |
                                 QSPI_INSTRFRAME_DUMMYLEN(0) |
                                 QSPI_INSTRFRAME_TFRTYPE_READ;

    // Trigger the transaction.
    // For instruction mode, you often need to "clock out" the result by writing dummy data.
    // Read three bytes for the JEDEC ID.
    uint8_t jedecId[3];
    for (int i = 0; i < 3; i++)
    {
        // Write a dummy value to start clocking.
        QSPI_REGS->QSPI_TXDATA = 0x00;
        // Wait until the RX data register is filled.
        while ((QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_RXC_Msk) == 0)
        {
            // Optionally add a timeout here.
        }
        // Read the received byte.
        jedecId[i] = QSPI_REGS->QSPI_RXDATA;
    }

    // Optionally, print out or log the JEDEC ID via USART or another debug interface.
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
