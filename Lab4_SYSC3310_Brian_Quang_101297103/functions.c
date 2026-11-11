#include "msp.h"
#include "functions.h"

/* Button connections */
#define BUTTON1       (1U << 1)  /* P1.1 */
#define BUTTON2       (1U << 4)  /* P1.4 */
#define BUTTON_MASK   (BUTTON1 | BUTTON2)

/* LED connections */
#define RED_LED       (1U << 0)  /* P1.0 */
#define RGB_RED       (1U << 0)  /* P2.0 */
#define RGB_GREEN     (1U << 1)  /* P2.1 */
#define RGB_BLUE      (1U << 2)  /* P2.2 */
#define RGB_MASK      (RGB_RED | RGB_GREEN | RGB_BLUE)

/*
 * These variables are private to this file.
 * selected_led: 0 = red LED, 1 = RGB LED
 * rgb_state:    0 to 7, representing off and 7 colors
 */
static volatile unsigned int selected_led = 0;
static volatile unsigned int rgb_state = 0;

/* Simple software debounce delay.
 * Assumes the processor is running at approximately 3 MHz.
 */

static void debounce_delay(void)
{
    volatile unsigned int i;

    for (i = 0; i < 10000U; i++)
    {
        __NOP();
    }
}

/* Configure P1.1 and P1.4 as inputs with pull-up resistors. */
void configure_switches(void)
{
    P1->DIR &= ~BUTTON_MASK;   /* Input pins */
    P1->REN |= BUTTON_MASK;    /* Enable resistors */
    P1->OUT |= BUTTON_MASK;    /* Select pull-up resistors */
}

/* Configure the red and RGB LEDs as outputs, initially off. */
void configure_leds(void)
{
    P1->DIR |= RED_LED;
    P2->DIR |= RGB_MASK;

    P1->OUT &= ~RED_LED;
    P2->OUT &= ~RGB_MASK;
}

/* Configure switch interrupts at the port, NVIC, and CPU levels. */
void configure_interrupts(void)
{
    /* Detect a falling edge: button released HIGH, pressed LOW. */
    P1->IES |= BUTTON_MASK;

    /* Clear any old interrupt flags. */
    P1->IFG &= ~BUTTON_MASK;

    /* Enable interrupts for both buttons at Port 1. */
    P1->IE |= BUTTON_MASK;

    /* Enable Port 1 interrupt in the NVIC. */
    NVIC_EnableIRQ(PORT1_IRQn);

    /* Enable interrupts at the CPU level. */
    __enable_irq();
}

/*
 * Port 1 interrupt service routine.
 * Called automatically when either button is pressed.
 */
void PORT1_IRQHandler(void)
{
    unsigned int pending;

    /* Find which button caused the interrupt. */
    pending = P1->IFG & BUTTON_MASK;

    /* Temporarily disable both button interrupts while debouncing. */
    P1->IE &= ~BUTTON_MASK;
    P1->IFG &= ~BUTTON_MASK;

    debounce_delay();

    /* Check Button 1 is still pressed (LOW). */
    if ((pending & BUTTON1) && !(P1->IN & BUTTON1))
    {
        /* Switch between red LED and RGB LED. */
        selected_led ^= 1U;
    }

    /* Check Button 2 is still pressed (LOW). */
    if ((pending & BUTTON2) && !(P1->IN & BUTTON2))
    {
        if (selected_led == 0)
        {
            /* Red LED: toggle its current state. */
            P1->OUT ^= RED_LED;
        }
        else
        {
            /* RGB LED: cycle through states 0 to 7. */
            rgb_state = (rgb_state + 1U) & 0x07U;

            /*
             * Clear only the RGB LED bits, leaving
             * all other Port 2 bits unchanged.
             */
            P2->OUT &= ~RGB_MASK;

            /* Set the bits corresponding to the new color. */
            P2->OUT |= (rgb_state & RGB_MASK);
        }
    }

    /* Allow time for switch contacts to settle. */
    debounce_delay();

    /* Clear flags that may have been set during debouncing. */
    P1->IFG &= ~BUTTON_MASK;

    /* Re-enable both button interrupts. */
    P1->IE |= BUTTON_MASK;
}

