#include "app.h"
#include "definitions.h"

/* Callbacks */
void toggle_user_led(){
    GPIO_PC18_Toggle();
}

/* bool verify_qspi_clocks() 
 *  char logBuffer[128];
 *  sprintf(logBuffer, verify_qspi_clocks() ? "[APP] QSPI Clocks verified successfully.\r\n" : "[ERROR] QSPI Clock verification failed!\r\n");
 *  SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
 *  while (SERCOM2_USART_WriteIsBusy());
 */
bool verify_qspi_clocks()
{
    // Read current clock settings
    uint32_t ahb_mask = MCLK_REGS->MCLK_AHBMASK;
    uint32_t gclk_qspi = GCLK_REGS->GCLK_PCHCTRL[39];

    // Check required clocks:
    // 1. CLK_QSPI_AHB     --> AHBMASK.QSPI bit must be set
    // 2. CLK_QSPI2X_AHB   --> AHBMASK.QSPI_2X bit must be set
    // 3. CLK_QSPI_APB     --> GCLK_PCHCTRL[39] must be enabled

    bool ahb_qspi_enabled     = (ahb_mask & MCLK_AHBMASK_QSPI_Msk);
    bool ahb_qspi2x_enabled   = (ahb_mask & MCLK_AHBMASK_QSPI_2X_Msk);
    bool gclk_qspi_enabled    = (gclk_qspi & GCLK_PCHCTRL_CHEN_Msk);

    return ahb_qspi_enabled && ahb_qspi2x_enabled && gclk_qspi_enabled;
    
}
/* 
 * bool verify_qspi_baud()
 * char logBuffer[128];  // Buffer to store log messages
 * sprintf(logBuffer, verify_qspi_baud() ? "[APP] QSPI Baud rate verified successfully.\r\n" : "[ERROR] QSPI Baud verification failed!\r\n");
 * SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
 * while (SERCOM2_USART_WriteIsBusy());
*/
bool verify_qspi_baud() {
    uint32_t qspi_baud = QSPI_REGS->QSPI_BAUD;

    // Extract clock polarity (CPOL bit)
    bool clock_polarity = (qspi_baud & QSPI_BAUD_CPOL_Msk) ? true : false;
    bool expected_polarity = false;

    if (clock_polarity != expected_polarity) {
        return false;
    }

    return true;
}

bool verify_qspi_registers()
{
    bool valid = true;

    // Check CTRLB config
    uint32_t ctrlb = QSPI_REGS->QSPI_CTRLB;
    uint32_t expected_ctrlb =
        QSPI_CTRLB_MODE_MEMORY |
        QSPI_CTRLB_CSMODE_NORELOAD |
        QSPI_CTRLB_DATALEN(0x6U);

    if ((ctrlb & (QSPI_CTRLB_MODE_Msk | QSPI_CTRLB_CSMODE_Msk | QSPI_CTRLB_DATALEN_Msk)) != expected_ctrlb)
    {
        valid = false;
    }

    // Check BAUD register (BAUD = 1 ? QSPI_CLK = Source / 4)
    if ((QSPI_REGS->QSPI_BAUD & QSPI_BAUD_BAUD_Msk) != QSPI_BAUD_BAUD(1U))
    {
        valid = false;
    }

    // Optionally check if QSPI was enabled
    if ((QSPI_REGS->QSPI_CTRLA & QSPI_CTRLA_ENABLE_Msk) == 0)
    {
        valid = false;
    }

    return valid;
}

/*
 * bool verify_qspi_registers_verbose()
 * char logBuffer[128];  // Buffer to store log messages            
 * sprintf(logBuffer, verify_qspi_registers_verbose() ? "[APP] QSPI Register setup verified.\r\n" : "[ERROR] QSPI Register setup mismatch!\r\n");
 * SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
 * while (SERCOM2_USART_WriteIsBusy());
*/
bool verify_qspi_registers_verbose()
{
    bool valid = true;
    char logBuffer[128];

    uint32_t ctrlb = QSPI_REGS->QSPI_CTRLB;
    uint32_t expected_ctrlb =
        QSPI_CTRLB_MODE_MEMORY |
        QSPI_CTRLB_CSMODE_NORELOAD |
        QSPI_CTRLB_DATALEN(0x6U);

    if ((ctrlb & (QSPI_CTRLB_MODE_Msk | QSPI_CTRLB_CSMODE_Msk | QSPI_CTRLB_DATALEN_Msk)) != expected_ctrlb)
    {
        sprintf(logBuffer, "[ERROR] CTRLB mismatch. Read: 0x%08lx\r\n", ctrlb);
        SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
        while (SERCOM2_USART_WriteIsBusy());
        valid = false;
    }

    uint32_t baud = QSPI_REGS->QSPI_BAUD;
    if ((baud & QSPI_BAUD_BAUD_Msk) != QSPI_BAUD_BAUD(1U))
    {
        sprintf(logBuffer, "[ERROR] BAUD mismatch. Read: 0x%08lx\r\n", baud);
        SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
        while (SERCOM2_USART_WriteIsBusy());
        valid = false;
    }

    uint32_t ctrla = QSPI_REGS->QSPI_CTRLA;
    if ((ctrla & QSPI_CTRLA_ENABLE_Msk) == 0)
    {
        sprintf(logBuffer, "[ERROR] CTRLA.ENABLE bit not set.\r\n");
        SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
        while (SERCOM2_USART_WriteIsBusy());
        valid = false;
    }

    return valid;
}

// Example function to send a JEDEC ID read command via QSPI in Instruction Mode.
void sendJEDECRead(void)
{
    // Set the instruction address to 0 if not used by the command.
    QSPI_REGS->QSPI_INSTRADDR = 0x00000000;

    // Configure the instruction control register:
    // - Set opcode to 0x9F (JEDEC ID read)
    // - Set address length to 0, option length to 0, and dummy cycles to 0
    QSPI_REGS->QSPI_INSTRCTRL =
            QSPI_INSTRCTRL_OPCODE(0x9F)  |  // Opcode for JEDEC read
            QSPI_INSTRCTRL_ADDRLEN_0     |  // No address bytes
            QSPI_INSTRCTRL_OPTLEN_0      |  // No option field
            QSPI_INSTRCTRL_DUMMYLEN(0);     // No dummy cycles

    // Configure the instruction frame register:
    // - Use single-bit width for instruction/data (for now)
    // - Enable the instruction and data phases
    // - Set transfer type to READ
    QSPI_REGS->QSPI_INSTRFRAME =
            QSPI_INSTRFRAME_WIDTH_SINGLE_BIT |   // Single-bit transfers
            QSPI_INSTRFRAME_INSTEN_Msk       |   // Enable instruction phase
            QSPI_INSTRFRAME_DATAEN_Msk       |   // Enable data phase
            QSPI_INSTRFRAME_TFRTYPE_READ;         // Read transfer

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
            // Call our JEDEC ID read function.
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
