#include "functions.h"
#include "inc/BSP.h"
#include "msp432p401r.h"

// Joystick thresholds
#define LOW_THRESHOLD   341
#define HIGH_THRESHOLD  682

// SMCLK after BSP_Clock_InitFastest()
#define SMCLK_FREQUENCY 12000000UL

#define MAX_RECORDED_NOTES 16U
#define PLAY_NOTE_US       300000UL
#define PLAY_GAP_US        50000UL

// A5 through A6 notes
static const uint16_t g_frequencies[9] =
{
    880,   // Quadrant 0: silent
    880,   // 1
    988,   // 2
    1109,  // 3
    1175,  // 4
    1319,  // 5
    1480,  // 6
    1661,  // 7
    1760   // 8
};

// RGB duty cycles: red, green, blue
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

// Recorder
static uint8_t g_recordedNotes[MAX_RECORDED_NOTES];
static uint8_t g_recordedCount;
static uint8_t g_isRecording;

// Playback
static uint8_t g_isPlaying;
static uint8_t g_playingNote;
static uint8_t g_playbackPhase; // 0 = note, 1 = silence
static uint32_t g_playbackDeadline;

// Current joystick quadrant
static uint8_t g_currentQuadrant;


// Set the buzzer frequency and RGB output
static void SetBuzzerTone(uint16_t frequency, uint16_t duty)
{
    uint32_t cycles = SMCLK_FREQUENCY / frequency;

    TA0CCR0 = cycles - 1;
    TA0CCR4 = (duty * cycles) >> 10;

    // Red and green share Timer A0 with the buzzer
    TA0CCR3 = (g_redDuty * cycles) >> 10;
    TA0CCR1 = (g_greenDuty * cycles) >> 10;

    // Blue uses Timer A2
    TA2CCR1 =
        (g_blueDuty * (SMCLK_FREQUENCY / 2048UL)) >> 10;

    TA0R = 0;
}


static void SetQuadrantOutput(uint8_t quadrant, uint16_t buzzerDuty)
{
    g_redDuty = g_rgbColors[quadrant][0];
    g_greenDuty = g_rgbColors[quadrant][1];
    g_blueDuty = g_rgbColors[quadrant][2];

    SetBuzzerTone(g_frequencies[quadrant], buzzerDuty);
}


static void StartPlaybackNote(uint32_t now)
{
    SetQuadrantOutput(g_recordedNotes[g_playingNote], 512);
    g_playbackPhase = 0;
    g_playbackDeadline = now + PLAY_NOTE_US;
}


void Instrument_Init(void)
{
    g_currentQuadrant = 255;
    g_recordedCount = 0;
    g_isRecording = 0;
    g_isPlaying = 0;
    g_playingNote = 0;
    g_playbackPhase = 0;
    g_playbackDeadline = 0;

    g_redDuty = 0;
    g_greenDuty = 0;
    g_blueDuty = 0;

    BSP_LCD_SetCursor(0, 0);
    BSP_LCD_DrawString(0, 0, "READY     ", LCD_GREEN);
}


// Pressed S1
void Instrument_RecordButton(void)
{
    if (g_isPlaying)
    {
        return;
    }

    if (g_isRecording)
    {
        g_isRecording = 0;

        BSP_LCD_SetCursor(0, 0);
        BSP_LCD_DrawString(0, 0, "STOPPED   ", LCD_GREEN);
    }
    else
    {
        g_recordedCount = 0;
        g_isRecording = 1;

        BSP_LCD_SetCursor(0, 0);
        BSP_LCD_DrawString(0, 0, "REC 00    ", LCD_RED);
    }
}


// Pressed S2
void Instrument_PlayButton(void)
{
    if (g_isPlaying || g_isRecording)
    {
        return;
    }

    if (g_recordedCount == 0)
    {
        BSP_LCD_SetCursor(0, 0);
        BSP_LCD_DrawString(0, 0, "NO NOTES  ", LCD_YELLOW);
        return;
    }

    g_isPlaying = 1;
    g_playingNote = 0;

    BSP_LCD_SetCursor(0, 0);
    BSP_LCD_DrawString(0, 0, "PLAY      ", LCD_CYAN);

    StartPlaybackNote(BSP_Time_Get());
}


// Determine joystick quadrant
uint8_t Joystick_GetQuadrant(uint16_t x, uint16_t y)
{
    // Top row
    if (y >= HIGH_THRESHOLD)
    {
        if (x < LOW_THRESHOLD)
            return 1;
        if (x < HIGH_THRESHOLD)
            return 2;
        return 3;
    }

    // Middle row
    if (y >= LOW_THRESHOLD)
    {
        if (x < LOW_THRESHOLD)
            return 4;
        if (x < HIGH_THRESHOLD)
            return 0;
        return 5;
    }

    // Bottom row
    if (x < LOW_THRESHOLD)
        return 6;
    if (x < HIGH_THRESHOLD)
        return 7;
    return 8;
}


// Main joystick/record/playback update
void Instrument_Update(void)
{
    uint32_t now;

    // Playback
    if (g_isPlaying)
    {
        now = BSP_Time_Get();

        if ((int32_t)(now - g_playbackDeadline) < 0)
        {
            return;
        }

        if (g_playbackPhase == 0)
        {
            // End 300 ms note and begin 50 ms silence
            SetQuadrantOutput(0, 0);
            g_playbackPhase = 1;
            g_playbackDeadline = now + PLAY_GAP_US;
        }
        else
        {
            g_playingNote++;

            if (g_playingNote >= g_recordedCount)
            {
                g_isPlaying = 0;
                g_currentQuadrant = 255;

                BSP_LCD_SetCursor(0, 0);
                BSP_LCD_DrawString(0, 0, "DONE      ", LCD_GREEN);
            }
            else
            {
                StartPlaybackNote(now);
            }
        }

        return;
    }

    // Normal joystick operation
    {
        uint16_t x;
        uint16_t y;
        uint8_t select;
        uint8_t newQuadrant;
        uint16_t duty;

        BSP_Joystick_Input(&x, &y, &select);
        (void)select;

        newQuadrant = Joystick_GetQuadrant(x, y);

        if (newQuadrant != g_currentQuadrant)
        {
            duty = (newQuadrant == 0) ? 0 : 512;

            SetQuadrantOutput(newQuadrant, duty);
            g_currentQuadrant = newQuadrant;

            // Record only movements into nonzero quadrants
            if (g_isRecording && newQuadrant != 0)
            {
                g_recordedNotes[g_recordedCount++] = newQuadrant;

                BSP_LCD_SetCursor(0, 0);
                BSP_LCD_DrawString(0, 0,
                                   (g_recordedCount >= 10)
                                   ? "REC " : "REC 0",
                                   LCD_RED);

                if (g_recordedCount < 10)
                {
                    BSP_LCD_OutUDec(g_recordedCount, LCD_RED);
                }
                else
                {
                    BSP_LCD_OutUDec(g_recordedCount, LCD_RED);
                }

                if (g_recordedCount >= MAX_RECORDED_NOTES)
                {
                    g_isRecording = 0;

                    BSP_LCD_SetCursor(0, 0);
                    BSP_LCD_DrawString(0, 0, "REC FULL ", LCD_RED);
                }
            }
        }
    }
}
