#include <stdint.h>
#include "my_functions.h"

void myFunction(void)
{
    uint8_t aByte;
    uint16_t aWord;

    aByte = 0x0F;                  // 1. aByte = 0x0F
    aByte |= 0x50;                 // 2. aByte = 0x5F
    aByte &= 0xFE;                 // 3. aByte = 0x5E
    aByte = ~((~aByte) | 0x04);    // 4. aByte = 0x5A
    aByte ^= 0xF0;                 // 5. aByte = 0xAA
    aByte >>= 2;                   // 6. aByte = 0x2A
    aByte <<= 2;                   // 7. aByte = 0xA8

    aWord = 0xF0F0;                // 8. aWord = 0xF0F0
    aWord |= 0xFF00;               // 9. aWord = 0xFFF0
    aWord |= aByte;                // 10. aWord = 0xFFF8
    aWord &= ~(aByte << 8);        // 11. aWord = 0x57F8; clears high-byte bits selected by aByte
}
