#include "app.h"
#include "definitions.h"

/* Read/Write Macros */
#define TEST_ADDRESS       0x00100000UL
#define PAGE_SIZE          256

/* JEDEC Values */
#define JEDEC_MANUFACTURER_ID  0x01
#define JEDEC_MEMORY_TYPE      0x02
#define JEDEC_CAPACITY_ID      0x20

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

void qspi_erase_256k_sector(uint32_t address)
{
    qspi_command_xfer_t eraseCmd = {
        .instruction = 0xDC,               // 256KB Sector Erase (4-byte command)
        .width = SINGLE_BIT_SPI,
        .addr_en = true,
        .addr_len = ADDRL_32_BIT
    };

    qspi_memory_write_enable();
    SERCOM2_USART_Write("[APP] Issuing 256KB Sector Erase...\r\n", 41);
    QSPI_CommandWrite(&eraseCmd, address);
    qspi_wait_for_write_complete();

    SERCOM2_USART_Write("[APP] Sector Erase Complete.\r\n", 32);
}

// --- Write Enable ---
void qspi_memory_write_enable(void)
{
    qspi_command_xfer_t writeEnable = {
        .instruction = 0x06,
        .width = SINGLE_BIT_SPI,
        .addr_en = false,
        .addr_len = ADDRL_24_BIT
    };

    QSPI_CommandWrite(&writeEnable, 0);
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

    qspi_memory_xfer_t writeTransfer = {
        .instruction = 0x02,
        .option = 0,
        .width = SINGLE_BIT_SPI,
        .addr_len = ADDRL_24_BIT,
        .option_en = false,
        .option_len = OPTL_1_BIT,
        .continuous_read_en = false,
        .dummy_cycles = 0
    };

    qspi_memory_write_enable();

    QSPI_MemoryWrite(&writeTransfer, (uint32_t *)data, (uint32_t)length, address);
}

// --- Memory Read ---
void qspi_memory_read_example(uint32_t address, uint8_t *buffer, size_t length)
{
    qspi_memory_xfer_t readTransfer = {
        .instruction = 0x6B,                // Fast Read Quad Output
        .option = 0,
        .width = QUAD_OUTPUT,
        .addr_len = ADDRL_24_BIT,
        .option_en = false,
        .option_len = OPTL_1_BIT,
        .continuous_read_en = false,
        .dummy_cycles = 8
    };

    QSPI_MemoryRead(&readTransfer, (uint32_t *)buffer, (uint32_t)length, address);
}

// --- Main Test Function ---
void test_qspi_write_and_read(void)
{
    uint8_t writeData[PAGE_SIZE] = {0};
    uint8_t readBack[PAGE_SIZE] = {0};

    // Fill write buffer with pattern
    for (uint16_t i = 0; i < PAGE_SIZE; i++)
    {
        writeData[i] = i & 0xFF;
    }

    qspi_memory_write_page(QSPI_TEST_ADDRESS, writeData, PAGE_SIZE);
    qspi_wait_for_write_complete();
    qspi_memory_read_example(QSPI_TEST_ADDRESS, readBack, PAGE_SIZE);

    // Compare and report
    bool match = memcmp(writeData, readBack, PAGE_SIZE) == 0;

    char logBuffer[64];
    sprintf(logBuffer, "[APP] QSPI Test: %s\r\n", match ? "PASS" : "FAIL");
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
            test_qspi_write_and_read();
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
