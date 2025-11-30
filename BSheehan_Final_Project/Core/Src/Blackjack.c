#include "Blackjack.h"
#include <stdlib.h>          // rand()
#include "LCD_Driver.h"      // your LCD API

// Rank text and suit characters
static const char *RankStrings[13] = {
    "A","2","3","4","5","6","7","8","9","10","J","Q","K"
};

static const char SuitChars[SUIT_COUNT] = {
    'C', 'D', 'H', 'S'       // Clubs, Diamonds, Hearts, Spades
};

// Convert 0–51 to Card struct
Card Blackjack_CardFromId(uint8_t id)
{
    Card c;
    c.suit = (CardSuit)(id / 13);        // 0–3
    c.rank = (CardRank)((id % 13) + 1);  // 1–13
    return c;
}

// Get a random card id (0–51)
uint8_t Blackjack_DrawCardId(void)
{
    // Seed rand() once somewhere in your init code, e.g.:
    // srand(HAL_GetTick());
    return (uint8_t)(rand() % 52);
}

// Draw a random card and return it
Card Blackjack_DrawCard(void)
{
    uint8_t id = Blackjack_DrawCardId();
    return Blackjack_CardFromId(id);
}

// Draw simple rectangle card with rank + suit text
void Blackjack_DrawCardOnScreen(Card card, uint16_t x, uint16_t y)
{
    uint16_t cardW = 40;
    uint16_t cardH = 60;

    // Card background (white)
    LCD_Fill_Recticle(x, y, cardW, cardH, LCD_COLOR_WHITE);

    // Card border (black)
    LCD_Draw_Rectangle(x, y, cardW, cardH, LCD_COLOR_BLACK);

    // Build label like "AH" or "10S"
    char buf[4]; // enough for "10S\0"
    const char *rankStr = RankStrings[card.rank - 1];
    char suitChar = SuitChars[card.suit];

    if (rankStr[1] != '\0') {
        // two-character rank ("10")
        buf[0] = rankStr[0];
        buf[1] = rankStr[1];
        buf[2] = suitChar;
        buf[3] = '\0';
    } else {
        // single-character rank
        buf[0] = rankStr[0];
        buf[1] = suitChar;
        buf[2] = '\0';
    }

    // Set text color and draw label near top-left of card
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_Draw_String(x + 4, y + 4, buf);
}
