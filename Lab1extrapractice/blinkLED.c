//*****************************************************************************
//
// Copyright (C) 2012 - 2016 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the
//  distribution.
//
//  Neither the name of Texas Instruments Incorporated nor the names of
//  its contributors may be used to endorse or promote products derived
//  from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// MSP432 main.c template - P1.0 port toggle
//
//*****************************************************************************

#include "msp.h"

int main(void)
{
    volatile uint32_t i;
		volatile uint32_t b;
		int idx;
		int delay;
	
		uint32_t student_id[9] = {1, 0, 1, 2, 9, 7, 1, 0, 3};   //Initialize an array for student number

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;  // Stop watchdog timer

    // The following code toggles P1.0 port
    P1->DIR |= BIT0;                             // Configure P1.0 as output
		P1->OUT &= ~BIT0;                            // Start the LED OFF
		
		// 2. Scanning the array for the number "1"
		for (idx = 0; idx < 10; idx++)
    {
        if (student_id[idx] == 1)
        {
            // Position = index + 1 (e.g., index 0 is position 1)
            uint32_t pos = idx + 1; 

            // Blink as many times as the position number
            // Each blink requires 1 turn ON and 1 turn OFF
            for (b = 0; b < pos; b++)
            {
                P1->OUT |= BIT0;              // LED ON
                for(i=50000; i>0; i--);      // Delay
                
                P1->OUT &= ~BIT0;             // LED OFF
                for(i=50000; i>0; i--);      // Delay
            }
						
						// Wait approximately 1 second between numbers
						for(delay = 0; delay < 4; delay++){
								for(i=100000; i>0; i--);
						}
        }
		}
		
		// 3. Blink 10 times to show completion
    for (b = 0; b < 10; b++)
    {
        P1->OUT |= BIT0;                  // LED ON
        for(i=50000; i>0; i--);          // Delay
        
        P1->OUT &= ~BIT0;                 // LED OFF
        for(i=50000; i>0; i--);          // Delay
    }
		
    while(1)
    {
    }
}
