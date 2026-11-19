
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

// RGB duties (10-bit): red, green, blue.
// Quadrant 0 is off; quadrants 1-8 each have a distinct color.
static const uint16_t g_rgbColors[9][3] =
{
    {   0,    0,    0}, // 0: off
    {1023,    0,    0}, // 1: red
    {1023,  512,    0}, // 2: orange
    {1023, 1023,    0}, // 3: yellow
    {   0, 1023,    0}, // 4: green
    {   0, 1023, 1023}, // 5: cyan
    {   0,    0, 1023}, // 6: blue
    { 512,    0, 1023}, // 7: purple
    {1023, 1023, 1023}  // 8: white
};

static uint16_t g_redDuty;
static uint16_t g_greenDuty;
static uint16_t g_blueDuty;

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

    // Red and green share Timer A0 with the buzzer, so scale their
    // compare values to the current buzzer period.
    TA0CCR3 = (g_redDuty * cycles) >> 10;
    TA0CCR1 = (g_greenDuty * cycles) >> 10;

    // Blue uses Timer A2, whose PWM period remains 12 MHz / 2048.
    TA2CCR1 = (g_blueDuty * (SMCLK_FREQUENCY / 2048UL)) >> 10;

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

        g_redDuty = g_rgbColors[newQuadrant][0];
        g_greenDuty = g_rgbColors[newQuadrant][1];
        g_blueDuty = g_rgbColors[newQuadrant][2];

        // Change buzzer frequency and duty cycle; this also refreshes
        // the red/green PWM values for the new timer period.
        SetBuzzerTone(g_frequencies[newQuadrant], duty);

        // Save the current quadrant.
        g_currentQuadrant = newQuadrant;
    }
}
