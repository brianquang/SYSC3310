
#include "msp432p401r.h"
#include "inc/BSP.h"
#include "functions.h"

int main(void)
{
    // Disable Watchdog Timer.
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    // Initialize system clock.
    BSP_Clock_InitFastest();

    // Initialize joystick.
    BSP_Joystick_Init();

    // Initialize buzzer with 0% duty cycle.
    BSP_Buzzer_Init(0);

    // Initialize FSM.
    Instrument_Init();

    // Main loop.
    while (1)
    {
        Instrument_Update();
    }
}
