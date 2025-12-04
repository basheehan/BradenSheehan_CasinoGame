#include "GameDriver.h"

static AppState_t gAppState = APP_STATE_HOME;
static uint32_t   gLastTouchTime = 0;

static STMPE811_TouchData gTouchData;

static Button_t gHomeBlackjackBtn = {0,   200, 120, 100};
static Button_t gHomeRouletteBtn  = {120, 200, 120, 100};

static Button_t gBlackjackExitBtn = {180, 0, 60, 40};
static Button_t gRouletteExitBtn  = {180, 0, 60, 40};

static Button_t gBlackjackStartBtn = {40, 200, 160, 100};
static Button_t gRouletteStartBtn  = {40, 200, 160, 100};

static uint8_t gBlackjackRoundActive = 0;



void GameDriver_Init(void)
{
    gTouchData.orientation = STMPE811_Orientation_Portrait_2;

    gAppState      = APP_STATE_HOME;
    gLastTouchTime = 0;

    LCD_SetFont(&Font12x12);
    Bank_Init();
    CheatCode_Init();

    DrawHomeScreen();
}

void GameDriver_MainLoop(void)
{
    while (1)
    {
        if (gAppState == APP_STATE_HOME)
        {
            uint8_t pressed =
                (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1u : 0u;
            CheatCode_Update(pressed, HAL_GetTick());
        }

        STMPE811_State_t tstate = returnTouchStateAndLocation(&gTouchData);

        if (tstate == STMPE811_State_Pressed)
        {
            uint32_t now = HAL_GetTick();

            if ((now - gLastTouchTime) >= TOUCH_DEBOUNCE_MS)
            {
                uint16_t tx = gTouchData.x;
                uint16_t ty = 319 - gTouchData.y;  // flip Y

                switch (gAppState)
                {
                case APP_STATE_HOME:
                    if (Button_HitTest(&gHomeBlackjackBtn, tx, ty))
                    {
                        Blackjack_InitGame();
                        gAppState = APP_STATE_BLACKJACK;
                        DrawBlackjackScreen();
                    }
                    else if (Button_HitTest(&gHomeRouletteBtn, tx, ty))
                    {
                        gAppState = APP_STATE_ROULETTE;
                        DrawRouletteScreen();
                    }
                    break;

                case APP_STATE_BLACKJACK:
                    if (Button_HitTest(&gBlackjackExitBtn, tx, ty))
                    {
                        gAppState = APP_STATE_HOME;
                        DrawHomeScreen();
                    }
                    else if (!gBlackjackRoundActive &&
                             Button_HitTest(&gBlackjackStartBtn, tx, ty))
                    {
                        if (!Bank_CanBet())
                        {
                            LCD_SetTextColor(LCD_COLOR_WHITE);
                            LCD_Draw_String(20, 60, "Balance < $100");
                            break;
                        }

                        gBlackjackRoundActive = 1;
                        DrawBlackjackRoundScreen();
                    }
                    else if (gBlackjackRoundActive)
                    {
                        Blackjack_HandleTouch(tx, ty);
                    }
                    break;

                case APP_STATE_ROULETTE:
                    if (Button_HitTest(&gRouletteExitBtn, tx, ty))
                    {
                        gAppState = APP_STATE_HOME;
                        Roulette_InitGame();
                        DrawHomeScreen();
                    }
                    else
                    {
                        Roulette_HandleTouch(tx, ty);
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

//button helpers

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

//draw functions

static void DrawHomeScreen(void)
{
    LCD_Clear(LCD_COLOR_BLACK);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(40, 40, "Casino");

    char buf[24];
    snprintf(buf, sizeof(buf), "Balance: $%ld", (long)Bank_GetBalance());
    LCD_Draw_String(40, 60, buf);

    LCD_Draw_Button(
        gHomeBlackjackBtn.x, gHomeBlackjackBtn.y,
        gHomeBlackjackBtn.w, gHomeBlackjackBtn.h,
        LCD_COLOR_GREEN, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Blackjack");

    LCD_Draw_Button(
        gHomeRouletteBtn.x, gHomeRouletteBtn.y,
        gHomeRouletteBtn.w, gHomeRouletteBtn.h,
        LCD_COLOR_RED, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Roulette");
}

static void DrawBlackjackScreen(void)
{
    gBlackjackRoundActive = 0;
    Blackjack_Reset();

    LCD_Clear(LCD_COLOR_DARKGREEN);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Blackjack");

    LCD_Draw_Button(
        gBlackjackStartBtn.x, gBlackjackStartBtn.y,
        gBlackjackStartBtn.w, gBlackjackStartBtn.h,
        LCD_COLOR_LIGHTGREEN, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Start Round");

    LCD_Draw_Button(
        gBlackjackExitBtn.x, gBlackjackExitBtn.y,
        gBlackjackExitBtn.w, gBlackjackExitBtn.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Exit");
}

static void DrawBlackjackRoundScreen(void)
{
    gBlackjackRoundActive = 1;

    Blackjack_StartNewRound();
    Blackjack_DrawTable();
    Blackjack_DrawHands();

    LCD_Draw_Button(
        gBlackjackExitBtn.x, gBlackjackExitBtn.y,
        gBlackjackExitBtn.w, gBlackjackExitBtn.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Exit");
}

static void DrawRouletteScreen(void)
{
    Roulette_DrawTable();

    LCD_Draw_Button(
        gRouletteExitBtn.x, gRouletteExitBtn.y,
        gRouletteExitBtn.w, gRouletteExitBtn.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Exit");
}
