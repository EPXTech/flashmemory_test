#include "app.h"
#include "definitions.h"

/* Callbacks */
void toggle_user_led(){
    GPIO_PC18_Toggle();
}

/* Support Functions */
bool verify_qspi_clocks(){
    // QSPI Clock Verification
    uint32_t apb_mask = MCLK_REGS->MCLK_APBAMASK;
    uint32_t ahb_mask = MCLK_REGS->MCLK_AHBMASK;
    uint32_t gclk_qspi = GCLK_REGS->GCLK_PCHCTRL[39];

    // Assume clock verification must pass for initialization
    if ((apb_mask & MCLK_APBCMASK_QSPI_Msk) &&
        (ahb_mask & MCLK_AHBMASK_QSPI_Msk) &&
        (ahb_mask & MCLK_AHBMASK_QSPI_2X_Msk) &&
        (gclk_qspi & GCLK_PCHCTRL_CHEN_Msk))
    {
        return true;
    } else {
        return false;
    }
}

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
            if (!verify_qspi_clocks()) break;
            if (!verify_qspi_baud()) break;
            
            appData.state = APP_STATE_RUNNING;
        }
        case APP_STATE_RUNNING:
        {
            // Application's main loop logic
            TC0_Timer16bitCounterSet(15000); // 500ms = 29295
            break;
        }
        case APP_STATE_ERROR:
        {
            // Handle errors (could add a debug message)
            TC0_Timer16bitCounterSet(60000); // 500ms = 29295
            break;
        }
        default:
        {
            break;
        }
    }
    
    
}
