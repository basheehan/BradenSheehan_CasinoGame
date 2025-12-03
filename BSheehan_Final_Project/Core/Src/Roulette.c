#include "Roulette.h"
#include "LCD_Driver.h"
#include "fonts.h"
#include "Bank.h"
#include "CheatCode.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------
//  Roulette game state
// ---------------------

typedef enum
{
    ROULETTE_STATE_IDLE = 0, // waiting for color selection
    ROULETTE_STATE_RESULT    // last spin result is shown
} RouletteState_t;

typedef enum
{
    ROULETTE_COLOR_NONE = 0,
    ROULETTE_COLOR_RED,
    ROULETTE_COLOR_BLACK
} RouletteColor_t;

typedef struct
{
    RouletteState_t state;
    RouletteColor_t selectedColor; // player’s chosen color (Red/Black)
    int8_t selectedNumber;         // -2 = none, -1 = 00, 0 = 0, 1–15 = numbers
    int8_t lastResult;             // -1 = 00, 0 = 0, 1–15 = numbers
    char statusText[40];           // status line buffer
} RouletteGame_t;

#define ROULETTE_NO_NUMBER ((int8_t)-2)

static RouletteGame_t gRoulette;

// ---------------------
//  Button definitions
// ---------------------

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} ROU_Button_t;

// x, y, w, h (same style as your Blackjack buttons)
static const ROU_Button_t sBtnBet = {0, 270, 80, 45};
static const ROU_Button_t sBtnRed = {80, 270, 80, 45};
static const ROU_Button_t sBtnBlack = {160, 270, 80, 45};

static const uint8_t sNumberGrid[ROULETTE_ROWS][ROULETTE_COLS] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
    {10, 11, 12},
    {13, 14, 15},
    {16, 17, 18},
    {19, 20, 21},
    {22, 23, 24},
    {25, 26, 27},
    {28, 29, 30},
    {31, 32, 33},
    {34, 35, 36}};

// Helper: standard red-number list (1–36)
// Return 1 if number should be red, 0 if black.
// Spec: 1 is red, 2 black, alternating up to 15.
static uint8_t Roulette_IsRed(uint8_t n)
{
    if (n < 1u || n > 15u)
    {
        return 0u;
    }

    // 1,3,5,... = red; 2,4,6,... = black
    return (uint8_t)(n % 2u); // odd -> 1, even -> 0
}

void Roulette_InitGame(void)
{
    gRoulette.state = ROULETTE_STATE_IDLE;
    gRoulette.selectedColor = ROULETTE_COLOR_NONE;
    gRoulette.selectedNumber = ROULETTE_NO_NUMBER; // NEW

    gRoulette.lastResult = 0;
    ROU_SetStatus("Select Red or Black");
}

static uint8_t ROU_ButtonHit(const ROU_Button_t *btn, uint16_t x, uint16_t y)
{
    return (x >= btn->x && x < (btn->x + btn->w) &&
            y >= btn->y && y < (btn->y + btn->h));
}
static uint8_t ROU_HitNumberCell(uint16_t x, uint16_t y, int8_t *outNumber)
{
    // Must match the grid geometry used in Roulette_DrawTable
    const uint16_t startX = 5;  // left margin of grid
    const uint16_t startY = 70; // top of row 0
    const uint16_t cellW = 77;  // width of each cell
    const uint16_t cellH = 32;  // height of each cell

    // Grid is 3 columns wide, 6 rows tall (row0: 0/00, rows1–5: 1–15)
    const uint16_t gridW = (uint16_t)(3u * cellW);
    const uint16_t gridH = (uint16_t)(6u * cellH);

    if (x < startX || x >= (uint16_t)(startX + gridW) ||
        y < startY || y >= (uint16_t)(startY + gridH))
    {
        return 0u;
    }

    uint16_t relX = (uint16_t)(x - startX);
    uint16_t relY = (uint16_t)(y - startY);

    uint8_t col = (uint8_t)(relX / cellW); // 0..2
    uint8_t row = (uint8_t)(relY / cellH); // 0..5

    if (col > 2u || row > 5u)
    {
        return 0u;
    }

    if (row == 0u)
    {
        // Row 0: 0, 00, blank
        if (col == 0u)
        {
            *outNumber = 0; // "0"
            return 1u;
        }
        else if (col == 1u)
        {
            *outNumber = -1; // "00"
            return 1u;
        }
        else
        {
            return 0u; // rightmost cell is blank
        }
    }
    else
    {
        // Rows 1–5: 1–15 laid out row-major
        // row1: 1,2,3 ; row2: 4,5,6 ; ... ; row5: 13,14,15
        uint8_t n = (uint8_t)(((row - 1u) * 3u) + col + 1u); // 1..15

        if (n >= 1u && n <= 15u)
        {
            *outNumber = (int8_t)n;
            return 1u;
        }
    }

    return 0u;
}

static void ROU_SetStatus(const char *msg)
{
    uint8_t i = 0;
    if (msg == NULL)
    {
        gRoulette.statusText[0] = '\0';
        return;
    }

    while (msg[i] != '\0' && i < (sizeof(gRoulette.statusText) - 1u))
    {
        gRoulette.statusText[i] = msg[i];
        i++;
    }
    gRoulette.statusText[i] = '\0';
}
// Draw status text in up to two lines, safely on-screen
// Draw status text in up to two lines, safely on-screen
static void ROU_DrawStatus(void)
{
    const char *msg = gRoulette.statusText;
    char line1[21];
    char line2[21];

    uint8_t maxPerLine = 18; // ~18 chars per 240px with 12x12 font
    uint8_t len = 0;

    while (msg[len] != '\0' && len < sizeof(gRoulette.statusText))
    {
        len++;
    }

    uint8_t firstLen = 0;
    uint8_t secondLen = 0;
    uint8_t start2 = 0;

    if (len <= maxPerLine)
    {
        firstLen = len;
        secondLen = 0;
    }
    else
    {
        // Try to break at a space before maxPerLine
        uint8_t breakPos = maxPerLine;
        while (breakPos > 0 && msg[breakPos] != ' ')
        {
            breakPos--;
        }

        if (breakPos == 0)
        {
            // No space found, hard break
            breakPos = maxPerLine;
        }

        firstLen = breakPos;

        // Skip spaces at start of second line
        start2 = breakPos;
        while (start2 < len && msg[start2] == ' ')
        {
            start2++;
        }

        uint8_t remaining = (uint8_t)(len - start2);
        if (remaining > maxPerLine)
        {
            remaining = maxPerLine;
        }
        secondLen = remaining;
    }

    // Copy first line
    uint8_t i;
    for (i = 0; i < firstLen && i < (sizeof(line1) - 1); i++)
    {
        line1[i] = msg[i];
    }
    line1[i] = '\0';

    // Copy second line (if any)
    if (secondLen > 0)
    {
        uint8_t j;
        for (j = 0; j < secondLen && j < (sizeof(line2) - 1); j++)
        {
            line2[j] = msg[start2 + j];
        }
        line2[j] = '\0';
    }
    else
    {
        line2[0] = '\0';
    }

    // Draw both lines in a safe band below the title (y ≈ 40–68)
    LCD_SetTextColor(LCD_COLOR_WHITE);

    // Clear a band for both lines (fully on-screen: 40..68)
    LCD_Fill_Rectangle(0, 40, 240, 28, LCD_COLOR_DARKGREEN);

    // First line
    LCD_Draw_String(10, 42, line1);

    // Second line (if present)
    if (line2[0] != '\0')
    {
        LCD_Draw_String(10, 56, line2);
    }
}

static RouletteColor_t Roulette_GetResultColor(int8_t result)
{
    if (result >= 1 && result <= 15)
    {
        return Roulette_IsRed((uint8_t)result) ? ROULETTE_COLOR_RED
                                               : ROULETTE_COLOR_BLACK;
    }

    // 0 or 00 have no color
    return ROULETTE_COLOR_NONE;
}

// Spin wheel: return -1 (00), 0 (0), or 1–15.
// If cheat is armed and a color is selected, force a winning number.
static int8_t Roulette_SpinWheel(void)
{
    int8_t result = 0;

    if (CheatCode_IsArmed())
    {
        uint8_t hasNumber = (gRoulette.selectedNumber != ROULETTE_NO_NUMBER);
        uint8_t hasColor = (gRoulette.selectedColor == ROULETTE_COLOR_RED ||
                            gRoulette.selectedColor == ROULETTE_COLOR_BLACK);

        if (hasNumber)
        {
            // Exact number cheat: 0, 00, or 1–15
            result = gRoulette.selectedNumber;
        }
        else if (hasColor)
        {
            // Force a winning number: pick a 1–15 that matches selectedColor
            uint8_t candidates[15];
            uint8_t count = 0;

            for (uint8_t n = 1; n <= 15; n++)
            {
                RouletteColor_t c = Roulette_GetResultColor((int8_t)n);
                if (c == gRoulette.selectedColor)
                {
                    candidates[count++] = n;
                }
            }

            if (count > 0u)
            {
                uint8_t idx = (uint8_t)(rand() % count);
                result = (int8_t)candidates[idx];
            }
            else
            {
                // Fallback (should not really happen)
                result = 0;
            }
        }
        else
        {
            // Cheat armed but no valid bet (shouldn't happen because Bet
            // requires a selection). Fall back to normal random.
            int r = rand() % 17; // 0..16

            if (r == 0)
            {
                result = 0; // "0"
            }
            else if (r == 1)
            {
                result = -1; // "00"
            }
            else
            {
                result = (int8_t)(r - 1); // 2..16 -> 1..15
            }
        }

        CheatCode_Consume();
        return result;
    }

    // Normal random spin (no cheat)
    // 17 equally likely outcomes: {0, 00, 1–15}
    int r = rand() % 17; // 0..16

    if (r == 0)
    {
        result = 0; // "0"
    }
    else if (r == 1)
    {
        result = -1; // "00"
    }
    else
    {
        result = (int8_t)(r - 1); // 2..16 -> 1..15
    }

    return result;
}

// Draw the main roulette betting layout (no logic, no touch handling yet)
void Roulette_DrawTable(void)
{
    printf("Roulette_DrawTable: enter\r\n");

    // Background
    LCD_Clear(LCD_COLOR_DARKGREEN);

    // Title (same style as Blackjack)
    LCD_SetFont(&Font12x12);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Roulette");

    // ----------------------------
    //  Grid of 0, 00, and 1–15
    // ----------------------------
    //

    const uint16_t startX = 5;  // left margin of grid
    const uint16_t startY = 70; // top of row 0
    const uint16_t cellW = 77;  // width of each cell
    const uint16_t cellH = 32;  // height of each cell

    // Row 0: 0, 00, (blank)
    const char *row0[3] = {"0", "00", ""};

    // Rows 1–5: numbers 1–15
    // row1: 1,2,3 ; row2: 4,5,6 ; ... ; row5: 13,14,15
    uint8_t numberRows[5][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9},
        {10, 11, 12},
        {13, 14, 15}};

    // --- Draw row 0: "0" and "00" (no colors) ---
    for (uint8_t col = 0; col < 3u; col++)
    {
        const char *label = row0[col];
        if ((label == NULL) || (label[0] == '\0'))
        {
            continue; // skip blank cell
        }

        uint16_t x = (uint16_t)(startX + (uint16_t)col * cellW);
        uint16_t y = startY;

        // Green background, white border
        LCD_Fill_Rectangle(x, y, cellW, cellH, LCD_COLOR_DARKGREEN);
        LCD_Draw_Rectangle(x, y, cellW, cellH, LCD_COLOR_WHITE);

        // Center text like Blackjack (manual offsets)
        int cx = x + (cellW / 2);
        int cy = y + (cellH / 2);

        // label length (1 or 2 chars)
        uint8_t len = (uint8_t)((label[0] != '\0') + (label[1] != '\0'));
        int textHalfWidth = (len * 12) / 2; // 12 px per char approx

        int tx = cx - textHalfWidth;
        int ty = cy - 6; // 12/2

        LCD_SetTextColor(LCD_COLOR_WHITE);
        LCD_Draw_String((uint16_t)tx, (uint16_t)ty, label);
    }

    // --- Draw rows 1–5: numbers 1–15 with alternating colors ---
    for (uint8_t r = 0; r < 5u; r++)
    {
        for (uint8_t c = 0; c < 3u; c++)
        {
            uint8_t n = numberRows[r][c];

            uint16_t x = (uint16_t)(startX + (uint16_t)c * cellW);
            uint16_t y = (uint16_t)(startY + (uint16_t)(r + 1u) * cellH); // +1 to skip row0

            uint32_t bg = Roulette_IsRed(n) ? LCD_COLOR_RED : LCD_COLOR_BLACK;

            // Fill cell + border
            LCD_Fill_Rectangle(x, y, cellW, cellH, bg);
            LCD_Draw_Rectangle(x, y, cellW, cellH, LCD_COLOR_WHITE);

            // Center number text (Blackjack style)
            char buf[4];
            int len = snprintf(buf, sizeof(buf), "%u", (unsigned)n);
            if (len < 1)
            {
                continue;
            }

            int cx = x + (cellW / 2);
            int cy = y + (cellH / 2);

            int textHalfWidth = (len * 12) / 2; // 12 px per char
            int tx = cx - textHalfWidth;
            int ty = cy - 6;

            LCD_SetTextColor(LCD_COLOR_WHITE);
            LCD_Draw_String((uint16_t)tx, (uint16_t)ty, buf);
        }
    }

    // ----------------------------
    //  Color selection + Bet button
    // ----------------------------

    LCD_Draw_Button(
        sBtnBet.x, sBtnBet.y, sBtnBet.w, sBtnBet.h,
        LCD_COLOR_LIGHTGRAY, // background
        LCD_COLOR_BLACK,     // border
        LCD_COLOR_BLACK,     // text
        "Bet");

    // Red / Black buttons along the bottom
    // Red button (left)
    LCD_Draw_Button(
        sBtnRed.x, sBtnRed.y, sBtnRed.w, sBtnRed.h,
        LCD_COLOR_RED,
        LCD_COLOR_BLACK,
        LCD_COLOR_BLACK,
        "Red");

    // Black button (right)
    LCD_Draw_Button(
        sBtnBlack.x, sBtnBlack.y, sBtnBlack.w, sBtnBlack.h,
        LCD_COLOR_BLACK,
        LCD_COLOR_BLACK,
        LCD_COLOR_WHITE,
        "Black");

    ROU_DrawStatus();
    printf("Roulette_DrawTable: leave\r\n");
}

void Roulette_HandleTouch(uint16_t x, uint16_t y)
{
    printf("Roulette_HandleTouch: x=%u y=%u state=%d\r\n",
           (unsigned)x, (unsigned)y, (int)gRoulette.state);

    // First, check if the touch is on the number grid (0, 00, 1–15)
    {
        int8_t hitNumber;
        if (ROU_HitNumberCell(x, y, &hitNumber))
        {
            gRoulette.selectedNumber = hitNumber;

            if (hitNumber == 0 || hitNumber == -1)
            {
                // 0 or 00: no color
                gRoulette.selectedColor = ROULETTE_COLOR_NONE;

                if (hitNumber == 0)
                {
                    ROU_SetStatus("Bet on 0");
                }
                else
                {
                    ROU_SetStatus("Bet on 00");
                }
            }
            else
            {
                // 1–15: also set the inherent color and show both
                RouletteColor_t c = Roulette_GetResultColor(hitNumber);
                gRoulette.selectedColor = c;

                const char *colStr = (c == ROULETTE_COLOR_RED) ? "RED" : "BLACK";
                char msg[40];
                snprintf(msg, sizeof(msg), "Bet: %d %s", (int)hitNumber, colStr);
                ROU_SetStatus(msg);
            }

            gRoulette.state = ROULETTE_STATE_IDLE;
            Roulette_DrawTable();
            return;
        }
    }

    // Color selection buttons always work (pure color bet)
    if (ROU_ButtonHit(&sBtnRed, x, y))
    {
        gRoulette.selectedColor = ROULETTE_COLOR_RED;
        gRoulette.selectedNumber = ROULETTE_NO_NUMBER; // clear number bet
        ROU_SetStatus("Red selected");
        gRoulette.state = ROULETTE_STATE_IDLE;
        Roulette_DrawTable();
        return;
    }

    if (ROU_ButtonHit(&sBtnBlack, x, y))
    {
        gRoulette.selectedColor = ROULETTE_COLOR_BLACK;
        gRoulette.selectedNumber = ROULETTE_NO_NUMBER; // clear number bet
        ROU_SetStatus("Black selected");
        gRoulette.state = ROULETTE_STATE_IDLE;
        Roulette_DrawTable();
        return;
    }

    // Bet button: must have either a color or a number (or both for 1–15)
    if (ROU_ButtonHit(&sBtnBet, x, y))
    {
        uint8_t hasNumber = (gRoulette.selectedNumber != ROULETTE_NO_NUMBER);
        uint8_t hasColor = (gRoulette.selectedColor != ROULETTE_COLOR_NONE);

        if (!hasNumber && !hasColor)
        {
            ROU_SetStatus("Pick color or number first");
            Roulette_DrawTable();
            return;
        }

        if (!Bank_CanBet())
        {
            ROU_SetStatus("Balance < $100");
            Roulette_DrawTable();
            return;
        }

        // Spin the wheel
        int8_t result = Roulette_SpinWheel();
        gRoulette.lastResult = result;

        RouletteColor_t resultColor = Roulette_GetResultColor(result);

        // Build a result string: "Spin: 7 RED - ..."
        char numBuf[4];
        const char *numStr;

        if (result == 0)
        {
            numStr = "0";
        }
        else if (result == -1)
        {
            numStr = "00";
        }
        else
        {
            snprintf(numBuf, sizeof(numBuf), "%d", (int)result);
            numStr = numBuf;
        }

        char msg[40];

        // Decide win type
        uint8_t hit = 0u;
        uint8_t numberHit = 0u;

        if (hasNumber && (result == gRoulette.selectedNumber))
        {
            // Exact number hit (0, 00, or 1–15)
            hit = 1u;
            numberHit = 1u;
        }
        else if (!hasNumber && hasColor && (resultColor == gRoulette.selectedColor))
        {
            // Color-only bet and color matches
            hit = 1u;
        }

        if (hit)
        {
            if (numberHit)
            {
                // Number bet hit: pay more (call win twice to approximate triple)
                Bank_ApplyRouletteNumberWin();
                snprintf(msg, sizeof(msg), "Spin: %s - Number hit!", numStr);
            }
            else
            {
                // Color-only win
                Bank_ApplyRouletteColorWin();
                const char *colStr =
                    (gRoulette.selectedColor == ROULETTE_COLOR_RED) ? "RED" : "BLACK";

                snprintf(msg, sizeof(msg), "Spin: %s %s - Color win!", numStr, colStr);
            }
        }
        else
        {
            Bank_ApplyLoss();
            snprintf(msg, sizeof(msg), "Spin: %s - House wins", numStr);
        }

        ROU_SetStatus(msg);
        gRoulette.state = ROULETTE_STATE_RESULT;

        Roulette_DrawTable();
        return;
    }
}
