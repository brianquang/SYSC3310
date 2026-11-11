#include <stdint.h>
#include "my_functions.h"

void myFunction(void)
{
    uint8_t aByte;
    uint16_t aWord;

    aByte = 0x0F;                     // Start with lower 4 bits turned on (0000 1111 in binary)
    aByte |= 0x50;                    // Turn on extra bits using OR (result: 0x5F)
    aByte &= 0xFE;                    // Turn off the very last bit using AND (result: 0x5E)
    aByte = ~((~aByte) | 0x04);       // Flip bits, turn on bit 2, then flip back (result: 0x5A)
    aByte ^= 0xF0;                    // Flip the upper 4 bits using XOR (result: 0xAA)
    aByte >>= 2;                      // Shift all bits 2 places to the right (result: 0x2A)
    aByte <<= 2;                      // Shift all bits 2 places back to the left (result: 0xA8)

    aWord = 0xF0F0;                   // Start with a 16-bit pattern (two bytes combined)
    aWord |= 0xFF00;                  // Force the entire upper byte to 1s using OR (result: 0xFFF0)
    aWord |= aByte;                   // Paste/combine aByte into the lower part of the 16-bit word
    aWord &= ~(aByte << 8);           // Shift aByte to the upper byte, flip it, and clear those bits in aWord
}
