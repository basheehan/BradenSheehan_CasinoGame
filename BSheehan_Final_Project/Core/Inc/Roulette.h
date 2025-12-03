#ifndef ROULETTE_H
#define ROULETTE_H

#include <stdint.h>
#include "LCD_Driver.h"
#include "fonts.h"

#include <stdint.h>
#include <stdio.h>
#include "Bank.h"
#include "CheatCode.h"
#include <stdlib.h>  

#define ROULETTE_ROWS 12
#define ROULETTE_COLS 3

void Roulette_InitGame(void);
void Roulette_DrawScreen(void);
static int8_t Roulette_SpinWheel(void);
static void ROU_SetStatus(const char *msg);
static void ROU_DrawStatus(void);

void Roulette_DrawTable(void);
void Roulette_HandleTouch(uint16_t x, uint16_t y);


#endif // ROULETTE_H
