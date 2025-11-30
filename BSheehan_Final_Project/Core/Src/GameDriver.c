#include "GameDriver.h"

static AppState_t gAppState = APP_STATE_HOME;
static uint32_t  gLastTouchTime = 0;

// Home screen buttons (same positions as LCD_Visual_Demo)
static Button_t gHomeBlackjackBtn = { 0,   200, 120, 100 };
static Button_t gHomeRouletteBtn  = { 120, 200, 120, 100 };

// Exit buttons (same spot for both games)
static Button_t gBlackjackExitBtn = { 80, 260, 160, 40 };
static Button_t gRouletteExitBtn  = { 80, 260, 160, 40 };

// forward declarations
static uint8_t Button_HitTest(const Button_t *btn, uint16_t x, uint16_t y);
static void    DrawHomeScreen(void);
static void    DrawBlackjackScreen(void);
static void    DrawRouletteScreen(void);

static void DebugPrintTouch(uint16_t x, uint16_t y)
{
    printf("Touch: x=%u y=%u\r\n", x, y);
}

void GameDriver_Init(void)
{
    // LCD already initialized in ApplicationInit()
    LCD_SetFont(&Font12x12);
    LCD_SetTextColor(LCD_COLOR_BLACK);

    gAppState = APP_STATE_HOME;
    DrawHomeScreen();
}

void GameDriver_MainLoop(void)
{
    while (1) {
#if COMPILE_TOUCH_FUNCTIONS == 1
        STMPE811_TouchData touch;
        STMPE811_State_t   state;

        state = returnTouchStateAndLocation(&touch);
        if (state == STMPE811_State_Pressed) {

            uint32_t now = HAL_GetTick();
            if ((now - gLastTouchTime) < TOUCH_DEBOUNCE_MS) {
                continue;
            }
            gLastTouchTime = now;

            // orient and convert to screen coordinates
            touch.orientation = STMPE811_Orientation_Portrait_2;
            DetermineTouchPosition(&touch);

            uint16_t x = touch.x;
            uint16_t y = touch.y;

            DebugPrintTouch(x, y);

            switch (gAppState) {

            case APP_STATE_HOME:
                if (Button_HitTest(&gHomeBlackjackBtn, x, y)) {
                    printf("Blackjack button hit\r\n");
                    gAppState = APP_STATE_BLACKJACK;
                    DrawBlackjackScreen();

                } else if (Button_HitTest(&gHomeRouletteBtn, x, y)) {
                    printf("Roulette button hit\r\n");
                    gAppState = APP_STATE_ROULETTE;
                    DrawRouletteScreen();
                }
                break;

            case APP_STATE_BLACKJACK:
                if (Button_HitTest(&gBlackjackExitBtn, x, y)) {
                    gAppState = APP_STATE_HOME;
                    DrawHomeScreen();
                }
                break;

            case APP_STATE_ROULETTE:
                if (Button_HitTest(&gRouletteExitBtn, x, y)) {
                    gAppState = APP_STATE_HOME;
                    DrawHomeScreen();
                }
                break;
            }
        }
#endif
    }
}

// ---------- helpers ----------

static uint8_t Button_HitTest(const Button_t *btn, uint16_t x, uint16_t y)
{
    if (x >= btn->x && x < (btn->x + btn->w) &&
        y >= btn->y && y < (btn->y + btn->h)) {
        return 1;
    }
    return 0;
}

static void DrawHomeScreen(void)
{
    LCD_Clear(LCD_COLOR_WHITE);

    // blackjack button
    LCD_Draw_Button(0, 200, 120, 100,
                    LCD_COLOR_LIGHTRED,   // bg
                    LCD_COLOR_BLACK,      // border
                    LCD_COLOR_BLACK,      // text
                    "BLACKJACK");

    // roulette button
    LCD_Draw_Button(120, 200, 120, 100,
                    LCD_COLOR_RED,
                    LCD_COLOR_BLACK,
                    LCD_COLOR_BLACK,
                    "ROULETTE");
}

static void DrawBlackjackScreen(void)
{
    LCD_Clear(LCD_COLOR_DARKGREEN);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Blackjack");

    LCD_Draw_Button(gBlackjackExitBtn.x, gBlackjackExitBtn.y,
                    gBlackjackExitBtn.w, gBlackjackExitBtn.h,
                    LCD_COLOR_LIGHTGRAY,   // bg
                    LCD_COLOR_BLACK,       // border
                    LCD_COLOR_BLACK,       // text
                    "Exit");
}

static void DrawRouletteScreen(void)
{
    LCD_Clear(LCD_COLOR_BLUE);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Roulette");

    LCD_Draw_Button(gRouletteExitBtn.x, gRouletteExitBtn.y,
                    gRouletteExitBtn.w, gRouletteExitBtn.h,
                    LCD_COLOR_LIGHTGRAY,   // bg
                    LCD_COLOR_BLACK,       // border
                    LCD_COLOR_BLACK,       // text
                    "Exit");
}
