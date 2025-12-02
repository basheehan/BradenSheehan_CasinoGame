#ifndef GAMEDRIVER_H_
#define GAMEDRIVER_H_

#include <stdint.h>
#include <stdio.h>        // for printf
#include "LCD_Driver.h"
#include "fonts.h"
#include "stmpe811.h"     // touch controller api
#include "stm32f4xx_hal.h"
#include "Blackjack.h"
#include "Bank.h"
#include "CheatCode.h"

#define TOUCH_DEBOUNCE_MS 200U

// button definition
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} Button_t;

// application states
typedef enum {
    APP_STATE_HOME = 0,
    APP_STATE_BLACKJACK,
    APP_STATE_ROULETTE
} AppState_t;

// main functions
void GameDriver_Init(void);
void GameDriver_MainLoop(void);

static uint8_t Button_HitTest(const Button_t *btn, uint16_t x, uint16_t y);
static void DrawHomeScreen(void);
static void DrawBlackjackScreen(void);
static void DrawRouletteScreen(void);
static void DebugPrintTouch(uint16_t x, uint16_t y, STMPE811_State_t state);
static void DrawBlackjackRoundScreen(void);

#endif
