
#include "functions.h"
#include "inc/BSP.h"
#include "msp432p401r.h"

// Joystick thresholds.
#define LOW_THRESHOLD  341
#define HIGH_THRESHOLD 682

// SMCLK frequency after BSP_Clock_InitFastest().
#define SMCLK_FREQUENCY 12000000UL

// Frequencies for each quadrant.
static const uint16_t g_frequencies[9] =
{
    880,   // Quadrant 0: silent
    880,   // Quadrant 1
    988,   // Quadrant 2
    1109,  // Quadrant 3
    1175,  // Quadrant 4
    1319,  // Quadrant 5
    1480,  // Quadrant 6
    1661,  // Quadrant 7
    1760   // Quadrant 8
};

// Current FSM state.
static uint8_t g_currentQuadrant;


// Change the buzzer frequency and duty cycle.
static void SetBuzzerTone(uint16_t frequency, uint16_t duty)
{
    uint32_t cycles;

    // Calculate the number of timer cycles for the frequency.
    cycles = SMCLK_FREQUENCY / frequency;

    // Set the timer period.
    TA0CCR0 = cycles - 1;

    // Set the duty cycle.
    // 512 = 50% duty cycle.
    // 0 = silent.
    TA0CCR4 = (duty * cycles) >> 10;

    // Restart the timer count for a clean transition.
    TA0R = 0;
}


// Initialize the instrument.
void Instrument_Init(void)
{
    g_currentQuadrant = 255;
}


// Determine which quadrant the joystick is in.
uint8_t Joystick_GetQuadrant(uint16_t x, uint16_t y)
{
    // Top row: quadrants 1, 2, 3.
    if (y >= HIGH_THRESHOLD)
    {
        if (x < LOW_THRESHOLD)
        {
            return 1;
        }
        else if (x < HIGH_THRESHOLD)
        {
            return 2;
        }
        else
        {
            return 3;
        }
    }

    // Middle row: quadrants 4, 0, 5.
    else if (y >= LOW_THRESHOLD)
    {
        if (x < LOW_THRESHOLD)
        {
            return 4;
        }
        else if (x < HIGH_THRESHOLD)
        {
            return 0;
        }
        else
        {
            return 5;
        }
    }

    // Bottom row: quadrants 6, 7, 8.
    else
    {
        if (x < LOW_THRESHOLD)
        {
            return 6;
        }
        else if (x < HIGH_THRESHOLD)
        {
            return 7;
        }
        else
        {
            return 8;
        }
    }
}


// Read joystick and update the buzzer.
void Instrument_Update(void)
{
    uint16_t x;
    uint16_t y;
    uint8_t select;

    uint8_t newQuadrant;
    uint16_t duty;

    // Read joystick position.
    BSP_Joystick_Input(&x, &y, &select);

    // Select button is not used in Part I.
    (void)select;

    // Find the current quadrant.
    newQuadrant = Joystick_GetQuadrant(x, y);

    // Only update when the quadrant changes.
    if (newQuadrant != g_currentQuadrant)
    {
        // Quadrant 0 is silent.
        if (newQuadrant == 0)
        {
            duty = 0;
        }
        else
        {
            // Other quadrants use 50% duty cycle.
            duty = 512;
        }

        // Change buzzer frequency and duty cycle.
        SetBuzzerTone(g_frequencies[newQuadrant], duty);

        // Save the current quadrant.
        g_currentQuadrant = newQuadrant;
    }
}
