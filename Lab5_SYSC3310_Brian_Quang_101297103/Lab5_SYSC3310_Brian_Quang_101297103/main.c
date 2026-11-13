#include "functions.h"

int main(void)
{
    /* Array of eight RGB color values */
    uint8_t colors[8] = {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U
    };

    uint8_t position = 0U;
    uint8_t red_led_active = 0U;
    uint32_t red_led_start_tick = 0U;
    uint32_t now;

    /* 1. Disable the watchdog timer */
    disable_watchdog();

    /* 2. Configure the hardware */
    initialize_leds();
    initialize_switches();
    initialize_timer();

    /* 3. Display the first color */
    set_rgb_color(colors[position]);

    /* 4. Enable global CPU interrupts */
    __enable_irq();

    /* 5. Run the game until SW2 is pressed */
    while (g_stop_requested == 0U)
    {
        

        if (take_reaction_success())
        {
            set_red_led(1U);
            red_led_active = 1U;
            red_led_start_tick = g_ticks;
        }

				now = g_ticks;
        if (red_led_active && ((uint32_t)(now - red_led_start_tick) >= RED_LED_DURATION_TICKS))
        {
            set_red_led(0U);
            red_led_active = 0U;
        }

        if ((uint32_t)(now - g_color_start_tick) >= COLOR_INTERVAL_TICKS)
        {
            position++;
            if (position >= 8U)
            {
                position = 0U;
            }
						/* Display the next color */
            set_rgb_color(colors[position]);
        }
    }

    /* SW2 pressed: shut down the timer and LEDs */
    stop_game_hardware();

    while (1)
    {
        /* End of game */
    }
}

