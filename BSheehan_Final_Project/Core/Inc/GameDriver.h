#ifndef GAMEDRIVER_H_
#define GAMEDRIVER_H_
#include <stdint.h>
#include <stdio.h>  // for printf
#include "LCD_Driver.h"
#include "fonts.h"
#include "stmpe811.h"   // touch controller api
#include "stm32f4xx_hal.h"
#include "ApplicationCode.h"

#define TOUCH_DEBOUNCE_MS 200U


//button stuff

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} Button_t;


//various screens
typedef enum {
    APP_STATE_HOME = 0,
    APP_STATE_BLACKJACK,
    APP_STATE_ROULETTE
} AppState_t;

void GameDriver_Init(void);
void GameDriver_MainLoop(void);



#endif
