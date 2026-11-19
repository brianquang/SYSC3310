
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>

void Instrument_Init(void);
void Instrument_Update(void);

uint8_t Joystick_GetQuadrant(uint16_t x, uint16_t y);

void Instrument_RecordButton(void);
void Instrument_PlayButton(void);

#endif
