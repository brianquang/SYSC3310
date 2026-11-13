#include "functions.h"

/* Shared variables */
volatile uint32_t g_ticks = 0;
volatile uint32_t g_color_start_tick = 0;

/* Bonus: adjustable color timing */
volatile uint32_t g_color_interval_ticks = COLOR_INTERVAL_TICKS;
volatile uint32_t g_active_color_interval_ticks = COLOR_INTERVAL_TICKS;

/* Record reaction and failure times */
volatile uint32_t g_reaction_success_time = 0;
volatile uint32_t g_failure_time = 0;

volatile uint8_t g_current_color = COLOR_OFF;
volatile uint8_t g_reaction_success = 0;
volatile uint8_t g_failure_event = 0;
volatile uint8_t g_stop_requested = 0;

/* Prevent multiple events for the same blue color */
static volatile uint8_t g_blue_reaction_recorded = 0;
static volatile uint8_t g_blue_timeout_recorded = 0;

/* =========================================
   1. DISABLE WATCHDOG
   ========================================= */
void disable_watchdog(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;
}

/* =========================================
   2. INITIALIZE LEDS
   ========================================= */
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

/* =========================================
   3. INITIALIZE SWITCHES
   ========================================= */
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

    /* Enable Port 1 interrupt in NVIC */
    NVIC_EnableIRQ(PORT1_IRQn);
}

/* =========================================
   4. INITIALIZE TIMER
   ========================================= */
void initialize_timer(void)
{
    /* Stop Timer A0 before configuring it */
    TIMER_A0->CTL = TIMER_A_CTL_MC__STOP | TIMER_A_CTL_CLR;

    /* Initialize bonus timing */
    g_ticks = 0;
    g_color_interval_ticks = COLOR_INTERVAL_TICKS;
    g_active_color_interval_ticks = COLOR_INTERVAL_TICKS;

    /* Reset event flags */
    g_reaction_success = 0;
    g_failure_event = 0;

    g_blue_reaction_recorded = 0;
    g_blue_timeout_recorded = 0;

    /* Select DCO as SMCLK */
    CS->KEY = CS_KEY_VAL;
    CS->CTL1 = (CS->CTL1 & ~CS_CTL1_SELS_MASK) | CS_CTL1_SELS__DCOCLK;
    CS->KEY = 0;

    /* 3000 cycles = 1 ms at 3 MHz */
    TIMER_A0->CCR[0] = 3000U - 1U;

    /* Enable CCR0 interrupt */
    TIMER_A0->CCTL[0] = TIMER_A_CCTLN_CCIE;

    /* Enable Timer A0 interrupt in NVIC */
    NVIC_ClearPendingIRQ(TA0_0_IRQn);
    NVIC_EnableIRQ(TA0_0_IRQn);

    /* Start Timer A0 in Up mode */
    TIMER_A0->CTL =
        TIMER_A_CTL_SSEL__SMCLK |
        TIMER_A_CTL_ID__1 |
        TIMER_A_CTL_MC__UP |
        TIMER_A_CTL_CLR;
}

/* =========================================
   5. SET RGB COLOR
   ========================================= */
void set_rgb_color(uint8_t color)
{
    color &= 0x07U;

    /* Record when this color starts */
    g_color_start_tick = g_ticks;

    /* Use the current adjustable speed */
    g_active_color_interval_ticks = g_color_interval_ticks;

    /* Reset reaction tracking for the new color */
    g_blue_reaction_recorded = 0;
    g_blue_timeout_recorded = 0;

    /* Display the RGB color */
    P2->OUT = (P2->OUT & ~(BIT0 | BIT1 | BIT2)) | ((uint16_t)color & (BIT0 | BIT1 | BIT2));

    g_current_color = color;
}

/* =========================================
   6. CONTROL RED LED
   ========================================= */
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

/* =========================================
   7. TURN OFF ALL LEDS
   ========================================= */
void turn_off_all_leds(void)
{
    P1->OUT &= ~BIT0;
    P2->OUT &= ~(BIT0 | BIT1 | BIT2);

    g_current_color = COLOR_OFF;
}

/* =========================================
   8. TAKE SUCCESS EVENT
   ========================================= */
uint8_t take_reaction_success(void)
{
    uint8_t result;

    __disable_irq();

    result = g_reaction_success;
    g_reaction_success = 0;

    __enable_irq();

    return result;
}

/* =========================================
   9. TAKE FAILURE EVENT
   ========================================= */
uint8_t take_failure_event(void)
{
    uint8_t result;

    __disable_irq();

    result = g_failure_event;
    g_failure_event = 0;

    __enable_irq();

    return result;
}

/* =========================================
   10. STOP GAME HARDWARE
   ========================================= */
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

/* =========================================
   11. TIMER A0 INTERRUPT
   ========================================= */
void TA0_0_IRQHandler(void)
{
    /* Clear timer interrupt flag */
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    /* Increase elapsed time by 1 ms */
    g_ticks++;

    /*
       BONUS:
       Check whether blue has expired without
       a successful SW1 reaction.
    */
    if (g_current_color == COLOR_BLUE &&
        !g_blue_reaction_recorded &&
        !g_blue_timeout_recorded &&
        (uint32_t)(g_ticks - g_color_start_tick) >= g_active_color_interval_ticks)
    {
        g_failure_event = 1U;
        g_failure_time = g_ticks;
        g_blue_timeout_recorded = 1U;
    }
}

/* =========================================
   12. PORT 1 INTERRUPT
   ========================================= */
void PORT1_IRQHandler(void)
{
    uint32_t pressed_switches;
    uint32_t press_time;

    /* Read which switches were pressed */
    pressed_switches = P1->IFG & (BIT1 | BIT4);

    /* Clear interrupt flags */
    P1->IFG &= ~(BIT1 | BIT4);

    /* Record current time */
    press_time = g_ticks;

    /* SW2: STOP GAME */
    if ((pressed_switches & BIT4) && !(P1->IN & BIT4))
    {
        g_stop_requested = 1U;
    }

    /* SW1: PLAYER REACTION */
    if ((pressed_switches & BIT1) && !(P1->IN & BIT1))
    {
        if (g_current_color == COLOR_BLUE &&
            !g_blue_reaction_recorded &&
            !g_blue_timeout_recorded &&
            !g_failure_event)
        {
            if ((uint32_t)(press_time - g_color_start_tick) < g_active_color_interval_ticks)
            {
                g_reaction_success_time = press_time;
                g_reaction_success = 1U;
                g_blue_reaction_recorded = 1U;
            }
        }
    }
}
