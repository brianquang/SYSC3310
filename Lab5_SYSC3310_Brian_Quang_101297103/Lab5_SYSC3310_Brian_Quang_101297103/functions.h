
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "msp.h"
#include <stdint.h>

/* Timer settings */
#define TIMER_TICK_MS           1U
#define COLOR_INTERVAL_TICKS    500U
#define RED_LED_DURATION_TICKS  250U

/* RGB LED colors */
#define COLOR_OFF    0U
#define COLOR_BLUE   4U

/* Shared variables updated by interrupts */
extern volatile uint32_t g_ticks;
extern volatile uint32_t g_color_start_tick;
extern volatile uint8_t g_current_color;
extern volatile uint8_t g_reaction_success;
extern volatile uint8_t g_stop_requested;

/* Hardware initialization */
void disable_watchdog(void);
void initialize_leds(void);
void initialize_switches(void);
void initialize_timer(void);

/* LED control */
void set_rgb_color(uint8_t color);
void set_red_led(uint8_t on);
void turn_off_all_leds(void);

/* Interrupt event handling */
uint8_t take_reaction_success(void);

/* Shutdown */
void stop_game_hardware(void);

#endif
