#include "app.h"
#include "definitions.h" // Include Harmony definitions for peripheral access

APP_DATA appData; // Application data structure

void APP_Initialize(void)
{
    // Initialize application state
    appData.state = APP_STATE_INIT;
}

void APP_Tasks(void)
{
    switch (appData.state)
    {
        case APP_STATE_INIT:
            // Perform startup logic (e.g., check hardware, configure settings)
            if (true)  // Example: Check if UART is ready
            {
                appData.state = APP_STATE_RUNNING;
            }
            break;

        case APP_STATE_RUNNING:
            // Application's main loop logic
            break;

        case APP_STATE_ERROR:
            // Handle errors (could add a debug message)
            break;

        default:
            break;
    }
}
