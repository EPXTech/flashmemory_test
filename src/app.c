#include "app.h"
#include "definitions.h"

/* Read/Write Macros */
#define TEST_ADDRESS       0x00100000UL
#define PAGE_SIZE          256

/* JEDEC Values */
#define JEDEC_MANUFACTURER_ID  0x01
#define JEDEC_MEMORY_TYPE      0x02
#define JEDEC_CAPACITY_ID      0x20

/* QSPI Config */
#define QSPI_DLYBS_VALUE 0x2

/* Callbacks */
void toggle_user_led(){
    GPIO_PC18_Toggle();
}

// Example function to send a JEDEC ID read command via QSPI in Instruction Mode.
void sendJEDECRead(void)
{
    uint8_t jedecId[3];
    uint32_t rxWord = 0;

    qspi_register_xfer_t readJEDECId =
    {
        .instruction = 0x9F,
        .width = SINGLE_BIT_SPI,
        .dummy_cycles = 0
    };

    bool success = QSPI_RegisterRead(&readJEDECId, &rxWord, 3);
    if (!success)
    {
        SERCOM2_USART_Write("[APP] QSPI JEDEC ID Read failed.\r\n", 36);
        while(SERCOM2_USART_WriteIsBusy() == true);
        return;
    }

    // Extract the 3 bytes
    jedecId[0] = (rxWord >> 0) & 0xFF;
    jedecId[1] = (rxWord >> 8) & 0xFF;
    jedecId[2] = (rxWord >> 16) & 0xFF;

    // Compare with expected
    bool match = (jedecId[0] == JEDEC_MANUFACTURER_ID) &&
                 (jedecId[1] == JEDEC_MEMORY_TYPE) &&
                 (jedecId[2] == JEDEC_CAPACITY_ID);

    // Format and print result
    char logBuffer[64];
    sprintf(logBuffer, "[APP] JEDEC ID: %02X %02X %02X => %s\r\n",
            jedecId[0], jedecId[1], jedecId[2],
            match ? "PASS" : "FAIL");

    SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
    while (SERCOM2_USART_WriteIsBusy());
}


// --- Write Enable ---
void qspi_memory_write_enable(void)
{
    qspi_command_xfer_t cmd = {
        .instruction = 0x06,              // Write Enable
        .width = SINGLE_BIT_SPI,
        .addr_en = false,
        .addr_len = ADDRL_24_BIT
    };
    QSPI_CommandWrite(&cmd, 0);
}

// --- Read Status Register ---
uint8_t qspi_read_status_register(void)
{
    uint32_t status = 0;
    qspi_register_xfer_t statusCmd = {
        .instruction = 0x05,
        .width = SINGLE_BIT_SPI,
        .dummy_cycles = 0
    };
    QSPI_RegisterRead(&statusCmd, &status, 1);
    return (uint8_t)(status & 0xFF);
}

// Log the current QSPI status register value to SERCOM2
void log_qspi_status(void)
{
    uint8_t status = qspi_read_status_register();
    char logBuffer[64];
    sprintf(logBuffer, "[APP] QSPI Status Register: 0x%02X\r\n", status);
    SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
    while (SERCOM2_USART_WriteIsBusy());
}

void qspi_wait_for_write_complete(void)
{
    while (qspi_read_status_register() & 0x01)
    {
        
    }
}

// --- Memory Write (Page Program) ---
void qspi_memory_write_page(uint32_t address, uint8_t *data, size_t length)
{
    if (length > PAGE_SIZE) length = PAGE_SIZE;

    qspi_memory_xfer_t writeXfer = {
        .instruction = 0x02,               // (0x12) Page Program with 4-byte address
        .option = 0,
        .width = SINGLE_BIT_SPI,
        .addr_len = ADDRL_24_BIT,
        .option_en = false,
        .option_len = OPTL_1_BIT,
        .continuous_read_en = false,
        .dummy_cycles = 0
    };

    qspi_memory_write_enable();
    SERCOM2_USART_Write("[APP] Writing page...\r\n", 24);
    while(SERCOM2_USART_WriteIsBusy() == true);
    QSPI_MemoryWrite(&writeXfer, (uint32_t *)data, (uint32_t)length, address);
    qspi_wait_for_write_complete();

    // Optionally add a small delay (e.g., a few ms) to let the flash settle.
    for (volatile int i = 0; i < 3000000; i++) { __asm__ __volatile__("nop"); }

    // Log the QSPI status register after programming.
    log_qspi_status();
}


// --- Memory Read ---
void qspi_memory_read(uint32_t address, uint8_t *buffer, size_t length)
{
    qspi_memory_xfer_t readXfer = {
        .instruction = 0x03,               // Normal Read command for single-bit mode
        .option = 0,
        .width = SINGLE_BIT_SPI,           // Force single-bit transactions
        .addr_len = ADDRL_24_BIT,
        .option_en = false,
        .option_len = OPTL_1_BIT,
        .continuous_read_en = false,
        .dummy_cycles = 0
    };

    SERCOM2_USART_Write("[APP] Reading back...\r\n", 24);
    while(SERCOM2_USART_WriteIsBusy() == true);
    QSPI_MemoryRead(&readXfer, (uint32_t *)buffer, (uint32_t)length, address);
}



void qspi_erase_256k_sector(uint32_t address)
{
    qspi_command_xfer_t eraseCmd = {
        .instruction = 0xDC,               // 256KB Erase in 4-byte mode
        .width = SINGLE_BIT_SPI,
        .addr_en = true,
        .addr_len = ADDRL_24_BIT
    };

    qspi_memory_write_enable();
    SERCOM2_USART_Write("[APP] Erasing 256KB sector...\r\n", 32);
    while(SERCOM2_USART_WriteIsBusy() == true);
    QSPI_CommandWrite(&eraseCmd, address);
    qspi_wait_for_write_complete();
    SERCOM2_USART_Write("[APP] Sector Erase Complete.\r\n", 32);
    while(SERCOM2_USART_WriteIsBusy() == true);
}

// --- Main Test Function ---
void test_qspi_read_write(void)
{
    uint8_t writeData[PAGE_SIZE], readData[PAGE_SIZE];
    char logBuffer[128];

    // Fill test pattern: start at 0x3C, increment by 4 for each byte.
    for (uint16_t i = 0; i < PAGE_SIZE; i++)
    {
        writeData[i] = (0x3C + 4 * i) & 0xFF;
    }

    // Erase target sector first
    qspi_erase_256k_sector(TEST_ADDRESS);

    // Write one page
    qspi_memory_write_page(TEST_ADDRESS, writeData, PAGE_SIZE);

    // Read back the data
    qspi_memory_read(TEST_ADDRESS, readData, PAGE_SIZE);

    // Verify and log results
    bool match = memcmp(writeData, readData, PAGE_SIZE) == 0;
    
    if (!match)
    {
        for (uint16_t i = 0; i < PAGE_SIZE; i++)
        {
            if (writeData[i] == readData[i])
            {
                sprintf(logBuffer, "[APP] ___MATCH at %u: wrote %02X, read %02X\r\n",
                        i, writeData[i], readData[i]);
                SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
                while(SERCOM2_USART_WriteIsBusy() == true);
//                break;
            }
            else //if (writeData[i] != readData[i])
            {
                sprintf(logBuffer, "[APP] MISMATCH at %u: wrote %02X, read %02X\r\n",
                        i, writeData[i], readData[i]);
                SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
                while(SERCOM2_USART_WriteIsBusy() == true);
//                break;
            }
        }
    }

    sprintf(logBuffer, "[APP] QSPI Test: %s\r\n", match ? "PASS" : "FAIL");
    SERCOM2_USART_Write(logBuffer, strlen(logBuffer));
    while (SERCOM2_USART_WriteIsBusy());
}


APP_DATA appData;

void APP_Initialize(void)
{
    // Initialize application state
    appData.state = APP_STATE_INIT;
    
    /* Start Flashing LED */
    TC0_TimerCallbackRegister(toggle_user_led, 0);
    TC0_TimerStart();
    
    /* Configure QSPI SCK Delay */
    QSPI_REGS->QSPI_BAUD = (QSPI_REGS->QSPI_BAUD & ~QSPI_BAUD_DLYBS_Msk) | QSPI_BAUD_DLYBS(QSPI_DLYBS_VALUE);
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
//            sendJEDECRead();
            test_qspi_read_write();

            // Optional delay if needed
            for (volatile int i = 0; i < 30000000; i++) { __asm__ __volatile__("nop"); }
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
