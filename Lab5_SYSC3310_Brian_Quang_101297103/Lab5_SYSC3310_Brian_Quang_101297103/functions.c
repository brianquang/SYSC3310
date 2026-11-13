

#include "functions.h"

/* Shared variables */
volatile uint32_t g_ticks = 0;
volatile uint32_t g_color_start_tick = 0;
volatile uint8_t g_current_color = COLOR_OFF;
volatile uint8_t g_reaction_success = 0;
volatile uint8_t g_stop_requested = 0;

/* Disable the watchdog timer */
void disable_watchdog(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;
}

/* Configure the separate red LED and RGB LED */
void initialize_leds(void)
{
    /* P1.0 = separate red LED */
    P1->SEL0 &= ~BIT0;
    P1->SEL1 &= ~BIT0;
    P1->DIR  |= BIT0;

    /* P2.0, P2.1, P2.2 = RGB LED */
    P2->SEL0 &= ~(BIT0 | BIT1 | BIT2);
    P2->SEL1 &= ~(BIT0 | BIT1 | BIT2);
    P2->DIR  |= BIT0 | BIT1 | BIT2;

    /* Start with all LEDs off */
    turn_off_all_leds();
}

/* Configure SW1 (P1.1) and SW2 (P1.4) */
void initialize_switches(void)
{
    /* Set both pins as GPIO */
    P1->SEL0 &= ~(BIT1 | BIT4);
    P1->SEL1 &= ~(BIT1 | BIT4);

    /* Configure both pins as inputs */
    P1->DIR &= ~(BIT1 | BIT4);

    /* Enable internal pull resistors */
    P1->REN |= BIT1 | BIT4;

    /* Select pull-up resistors */
    P1->OUT |= BIT1 | BIT4;

    /* Select falling-edge interrupts */
    P1->IES |= BIT1 | BIT4;

    /* Clear old interrupt flags */
    P1->IFG &= ~(BIT1 | BIT4);

    /* Enable interrupts for both switches */
    P1->IE |= BIT1 | BIT4;

    /* Enable Port 1 interrupt in the NVIC */
    NVIC_EnableIRQ(PORT1_IRQn);
}

/* Configure Timer A0 to generate one interrupt per millisecond.
   Assumes SMCLK = 3 MHz. */
void initialize_timer(void)
{
    /* Stop Timer_A0 before configuring it */
    TIMER_A0->CTL = TIMER_A_CTL_MC__STOP | TIMER_A_CTL_CLR;

    /* Clock config - keep this if your board is using the default 3 MHz DCO */
    CS->KEY = CS_KEY_VAL;
    CS->CTL1 = (CS->CTL1 & ~CS_CTL1_SELS_MASK) | CS_CTL1_SELS__DCOCLK;
    CS->KEY = 0;

    /* 3000 cycles = 1 ms at 3 MHz */
    TIMER_A0->CCR[0] = 3000U - 1U;

    /* Enable CCR0 interrupt */
    TIMER_A0->CCTL[0] = TIMER_A_CCTLN_CCIE;

    /* Enable Timer_A0 CCR0 interrupt in NVIC */
    NVIC_ClearPendingIRQ(TA0_0_IRQn);
    NVIC_EnableIRQ(TA0_0_IRQn);

    /* Start Timer_A0 in Up mode using SMCLK */
    TIMER_A0->CTL =
        TIMER_A_CTL_SSEL__SMCLK |
        TIMER_A_CTL_ID__1 |
        TIMER_A_CTL_MC__UP |
        TIMER_A_CTL_CLR;
}

/* Set the RGB LED using a color value from 0 to 7.
   Bit 0 = red, bit 1 = green, bit 2 = blue. */
void set_rgb_color(uint8_t color)
{
    color &= 0x07U;

    P2->OUT = (P2->OUT & ~(BIT0 | BIT1 | BIT2))
            | ((uint16_t)color & (BIT0 | BIT1 | BIT2));

    g_color_start_tick = g_ticks;
    g_current_color = color;
}

/* Turn the separate red LED on or off */
void set_red_led(uint8_t on)
{
    if (on)
    {
        P1->OUT |= BIT0;
    }
    else
    {
        P1->OUT &= ~BIT0;
    }
}

/* Turn off all LEDs */
void turn_off_all_leds(void)
{
    P1->OUT &= ~BIT0;
    P2->OUT &= ~(BIT0 | BIT1 | BIT2);

    g_current_color = COLOR_OFF;
}

/* Safely read and clear the reaction-success flag */
uint8_t take_reaction_success(void)
{
    uint8_t result;

    __disable_irq();
    result = g_reaction_success;
    g_reaction_success = 0;
    __enable_irq();

    return result;
}

/* Stop the timer and disable switch interrupts */
void stop_game_hardware(void)
{
    __disable_irq();

    /* Stop Timer A0 */
    TIMER_A0->CTL = TIMER_A_CTL_MC__STOP;

    /* Disable switch interrupts */
    P1->IE &= ~(BIT1 | BIT4);

    /* Disable timer interrupt */
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIE;

    /* Turn off every LED */
    turn_off_all_leds();

    /* Leave interrupts disabled */
}

/* Timer A0 CCR0 interrupt service routine */
void TA0_0_IRQHandler(void)
{
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    g_ticks++;
}

/* Port 1 interrupt service routine */
void PORT1_IRQHandler(void)
{
    uint32_t pressed_switches;
    uint32_t press_time;

    pressed_switches = P1->IFG & (BIT1 | BIT4);
    P1->IFG &= ~(BIT1 | BIT4);

    press_time = g_ticks;

    /* SW2: stop the game */
    if ((pressed_switches & BIT4) && !(P1->IN & BIT4))
    {
        g_stop_requested = 1;
    }

    /* SW1: check whether blue is displayed */
    if ((pressed_switches & BIT1) && !(P1->IN & BIT1))
    {
        if (g_current_color == COLOR_BLUE)
        {
            if ((uint32_t)(press_time - g_color_start_tick) < COLOR_INTERVAL_TICKS)
            {
                g_reaction_success = 1U;
            }
        }
    }
}

