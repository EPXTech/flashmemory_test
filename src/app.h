#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

// Define application states
typedef enum
{
    APP_STATE_INIT = 0,
    APP_STATE_RUNNING,
    APP_STATE_ERROR
} APP_STATES;

typedef struct
{
    APP_STATES state;
} APP_DATA;

extern APP_DATA appData;

// Function prototypes
void APP_Initialize(void);
void APP_Tasks(void);

#endif // APP_H
