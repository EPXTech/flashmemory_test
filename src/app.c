#include "app.h"
#include "definitions.h"

void toggle_user_led(){
    GPIO_PC18_Toggle();
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
            bool is_initialized = true;
            
            if (is_initialized)
            {
                appData.state = APP_STATE_RUNNING;
            }
            break;
        }
        case APP_STATE_RUNNING:
        {
            // Application's main loop logic
            
            break;
        }
        case APP_STATE_ERROR:
        {
            // Handle errors (could add a debug message)
            break;
        }
        default:
        {
            break;
        }
    }
}
