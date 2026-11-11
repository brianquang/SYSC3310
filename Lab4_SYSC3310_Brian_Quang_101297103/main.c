
#include "msp.h"
#include "functions.h"

int main(void)
{
    /* 1. Stop the watchdog timer. */
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    /* 2. Configure both buttons. */
    configure_switches();

    /* 3. Configure the red and RGB LEDs. */
    configure_leds();

    /* 4. Enable the button interrupts. */
    configure_interrupts();

    /* 5. Stay here; the ISR handles button presses. */
    while (1)
    {
        /* Nothing to do here. */
    }
}
