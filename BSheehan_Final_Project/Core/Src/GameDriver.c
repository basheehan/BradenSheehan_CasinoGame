#include "GameDriver.h"

static AppState_t gAppState = APP_STATE_HOME;
static uint32_t gLastTouchTime = 0;

// local touch state (same style as touch demo)
static STMPE811_TouchData gTouchData;

// Home screen buttons (match what you draw)
static Button_t gHomeBlackjackBtn = {0, 200, 120, 100};
static Button_t gHomeRouletteBtn = {120, 200, 120, 100};

// Exit buttons
static Button_t gBlackjackExitBtn = {180, 0, 60, 40};
static Button_t gRouletteExitBtn = {180, 0, 60, 40};

// start buttons

static Button_t gBlackjackStartBtn = {40, 200, 160, 100};
static Button_t gRouletteStartBtn = {40, 200, 160, 100};

static uint8_t gBlackjackRoundActive = 0; // used to track if inside a round

void GameDriver_Init(void)
{
    // Use same orientation as the LCD_Touch_Polling_Demo
    gTouchData.orientation = STMPE811_Orientation_Portrait_2;

    // Start at home screen
    gAppState = APP_STATE_HOME;
    gLastTouchTime = 0;
    LCD_SetFont(&Font12x12);

    DrawHomeScreen();
}

void GameDriver_MainLoop(void)
{
    while (1)
    {
        // Poll the touch controller using the SAME helper as the touch demo
        STMPE811_State_t tstate = returnTouchStateAndLocation(&gTouchData);

        if (tstate == STMPE811_State_Pressed)
        {
            uint32_t now = HAL_GetTick();

            // Simple debounce
            if ((now - gLastTouchTime) >= TOUCH_DEBOUNCE_MS)
            {
                // --- NEW: use local, LCD-aligned coordinates ---
                uint16_t tx = gTouchData.x;
                uint16_t ty = gTouchData.y;

                // Flip Y so 0 is at the TOP of the LCD
                ty = 319 - ty; // LCD height is 320 -> valid range 0..319

                DebugPrintTouch(tx, ty, tstate);

                switch (gAppState)
                {
                case APP_STATE_HOME:
                    if (Button_HitTest(&gHomeBlackjackBtn, tx, ty))
                    {
                        printf("Blackjack button hit\r\n");
                        Blackjack_InitGame(); // build + shuffle deck, set initial status
                        gAppState = APP_STATE_BLACKJACK;
                        DrawBlackjackScreen(); // show menu (Start Round + Exit)
                    }
                    else if (Button_HitTest(&gHomeRouletteBtn, tx, ty))
                    {
                        gAppState = APP_STATE_ROULETTE;
                        DrawRouletteScreen();
                    }
                    break;

                case APP_STATE_BLACKJACK:
                    // Exit button always works
                    if (Button_HitTest(&gBlackjackExitBtn, tx, ty))
                    {
                        printf("Blackjack: Exit button hit\r\n");
                        gAppState = APP_STATE_HOME;
                        DrawHomeScreen();
                    }
                    // If we’re on the menu (no round active yet)
                    else if (!gBlackjackRoundActive &&
                             Button_HitTest(&gBlackjackStartBtn, tx, ty))
                    {

                        printf("Blackjack: Start Round button hit\r\n");
                        gBlackjackRoundActive = 1;   

                        DrawBlackjackRoundScreen(); // now in active round view
                    }
                    // If a round is active, send touch to Blackjack engine for Hit/Stand/New
                    else if (gBlackjackRoundActive)
                    {
                        Blackjack_HandleTouch(tx, ty);
                    }
                    break;

                case APP_STATE_ROULETTE:
                    if (Button_HitTest(&gRouletteExitBtn, tx, ty))
                    {
                        gAppState = APP_STATE_HOME;
                        DrawHomeScreen();
                    }
                    break;

                default:
                    gAppState = APP_STATE_HOME;
                    DrawHomeScreen();
                    break;
                }

                gLastTouchTime = now;
            }
        }

        HAL_Delay(10);
    }
}

// ----------------- helpers -----------------

static uint8_t Button_HitTest(const Button_t *btn, uint16_t x, uint16_t y)
{
    if (!btn)
    {
        return 0;
    }

    if ((x >= btn->x) && (x < (btn->x + btn->w)) &&
        (y >= btn->y) && (y < (btn->y + btn->h)))
    {
        return 1;
    }

    return 0;
}

static void DebugPrintTouch(uint16_t x, uint16_t y, STMPE811_State_t state)
{
    (void)state; // state is not used for now, but kept for future debugging
    printf("Touch: x=%u y=%u\r\n", (unsigned)x, (unsigned)y);
}

// Simple UI drawing using your LCD helpers

static void DrawHomeScreen(void)
{
    LCD_Clear(LCD_COLOR_BLACK);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(40, 40, "Casino");

    // Blackjack button
    LCD_Draw_Button(
        gHomeBlackjackBtn.x, gHomeBlackjackBtn.y,
        gHomeBlackjackBtn.w, gHomeBlackjackBtn.h,
        LCD_COLOR_GREEN, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Blackjack");

    // Roulette button
    LCD_Draw_Button(
        gHomeRouletteBtn.x, gHomeRouletteBtn.y,
        gHomeRouletteBtn.w, gHomeRouletteBtn.h,
        LCD_COLOR_RED, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Roulette");
}

static void DrawBlackjackScreen(void)
{
    gBlackjackRoundActive = 0;

    // Optional: clear/reset internal blackjack state
    Blackjack_Reset();

    // Green background + title
    LCD_Clear(LCD_COLOR_DARKGREEN);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Blackjack");

    // "Start Round" button
    LCD_Draw_Button(
        gBlackjackStartBtn.x, gBlackjackStartBtn.y,
        gBlackjackStartBtn.w, gBlackjackStartBtn.h,
        LCD_COLOR_LIGHTGREEN, // fill
        LCD_COLOR_BLACK,      // border
        LCD_COLOR_BLACK,      // text
        "Start Round");

    // Exit button (top-right)
    LCD_Draw_Button(
        gBlackjackExitBtn.x, gBlackjackExitBtn.y,
        gBlackjackExitBtn.w, gBlackjackExitBtn.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Exit");
}
// drawing screen for mid round
static void DrawBlackjackRoundScreen(void)
{
    gBlackjackRoundActive = 1;

    // Draw the Blackjack table (background + Hit/Stand/New buttons)
    Blackjack_DrawTable();


    // Draw exit button on top of the table
    LCD_Draw_Button(
        gBlackjackExitBtn.x, gBlackjackExitBtn.y,
        gBlackjackExitBtn.w, gBlackjackExitBtn.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Exit");
}

static void DrawRouletteScreen(void)
{
    LCD_Clear(LCD_COLOR_BLUE);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Roulette");

    LCD_Draw_Button(
        gRouletteExitBtn.x, gRouletteExitBtn.y,
        gRouletteExitBtn.w, gRouletteExitBtn.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Exit");
}
