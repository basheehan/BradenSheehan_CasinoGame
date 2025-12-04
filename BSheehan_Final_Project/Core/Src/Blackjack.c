#include "Blackjack.h"

// ---------------------
//  Card rendering data
// ---------------------

static const char *RankStrings[13] = {
    "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

static const char SuitChars[SUIT_COUNT] = {
    'C', 'D', 'H', 'S'  // Clubs, Diamonds, Hearts, Spades
};

// ---------------------
//  Global game state
// ---------------------

static BlackjackGame_t gBJ;

// Simple button struct for Blackjack UI
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} BJ_Button_t;

// Hit / Stand / New Round buttons (240x320 portrait)
static const BJ_Button_t sBtnHit   = {  0, 270,  80, 45 };
static const BJ_Button_t sBtnStand = { 80, 270,  80, 45 };
static const BJ_Button_t sBtnNew   = {160, 270,  80, 45 };

// Forward declarations for internal helpers
static uint8_t BJ_IsTenValue(CardRank rank);
static void    BJ_ForceNextPlayerBlackjack(void);

// ---------------------
//  Button helpers
// ---------------------

static uint8_t BJ_ButtonHit(const BJ_Button_t *btn, uint16_t x, uint16_t y)
{
    return (x >= btn->x) &&
           (x < (btn->x + btn->w)) &&
           (y >= btn->y) &&
           (y < (btn->y + btn->h));
}

// ---------------------
//  Card helpers
// ---------------------

Card Blackjack_CardFromId(uint8_t id)
{
    Card c;
    c.suit = (CardSuit)(id / 13);
    c.rank = (CardRank)((id % 13) + 1);
    return c;
}

// Draw a raw card id (0–51)
static uint8_t BJ_RandomCardId(void)
{
    return (uint8_t)(rand() % 52);
}

// Draw a simple rectangle card with rank + suit text
void Blackjack_DrawCardOnScreen(Card card, uint16_t x, uint16_t y)
{
    uint16_t cardW = 40;
    uint16_t cardH = 60;

    // Card background
    LCD_Fill_Rectangle(x, y, cardW, cardH, LCD_COLOR_WHITE);

    // Card border
    LCD_Draw_Rectangle(x, y, cardW, cardH, LCD_COLOR_BLACK);

    // Build text like "AH" or "10S"
    char        buf[4];  // enough for "10S\0"
    const char *rankStr  = RankStrings[card.rank - 1];
    char        suitChar = SuitChars[card.suit];

    if (rankStr[1] != '\0') {
        // two-character rank (10)
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

    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_Draw_String(x + 4, y + 4, buf);
}

// ---------------------
//  Deck & hand helpers
// ---------------------

static void BJ_BuildDeck(void)
{
    uint8_t id = 0;

    for (uint8_t s = 0; s < SUIT_COUNT; s++) {
        for (uint8_t r = 0; r < 13; r++) {
            gBJ.deck[id].suit = (CardSuit)s;
            gBJ.deck[id].rank = (CardRank)(r + 1);
            id++;
        }
    }

    gBJ.deckIndex = 0;
}

static void BJ_ShuffleDeck(void)
{
    for (int i = 51; i > 0; i--) {
        int  j   = rand() % (i + 1);
        Card tmp = gBJ.deck[i];

        gBJ.deck[i] = gBJ.deck[j];
        gBJ.deck[j] = tmp;
    }

    gBJ.deckIndex = 0;
}

static Card BJ_DealCard(void)
{
    if (gBJ.deckIndex >= 52) {
        // Safety: rebuild & shuffle if we somehow run out
        BJ_BuildDeck();
        BJ_ShuffleDeck();
    }

    return gBJ.deck[gBJ.deckIndex++];
}

static int16_t BJ_HandValue(const Card *hand, uint8_t count)
{
    int16_t total = 0;
    uint8_t aces  = 0;

    for (uint8_t i = 0; i < count; i++) {
        uint8_t v;

        switch (hand[i].rank) {
        case RANK_ACE:
            v = 1;
            aces++;
            break;
        case RANK_JACK:
        case RANK_QUEEN:
        case RANK_KING:
            v = 10;
            break;
        default:
            v = (uint8_t)hand[i].rank;
            break;
        }

        total += v;
    }

    // Upgrade Aces from 1 -> 11 while it helps
    while ((aces > 0) && ((total + 10) <= 21)) {
        total += 10;
        aces--;
    }

    return total;
}

static void BJ_SetStatus(const char *msg)
{
    uint8_t i = 0;

    while ((msg[i] != '\0') && (i < (sizeof(gBJ.statusText) - 1U))) {
        gBJ.statusText[i] = msg[i];
        i++;
    }

    gBJ.statusText[i] = '\0';
}

// ---------------------
//  Core game control
// ---------------------

void Blackjack_StartRound(void)
{
    if (CheatCode_IsArmed()) {
        BJ_ForceNextPlayerBlackjack();
        CheatCode_Consume();
    }

    Blackjack_Reset();
    Blackjack_DrawTable();
    Blackjack_DrawHands();
    Blackjack_DrawStatus();
}

void Blackjack_Reset(void)
{
    // Clear player hand
    gBJ.playerCardCount = 0;
    gBJ.playerTotal     = 0;

    // Clear dealer hand
    gBJ.dealerCardCount = 0;
    gBJ.dealerTotal     = 0;

    // Reset game state to idle (pre-round)
    gBJ.state = BJ_STATE_IDLE;

    BJ_SetStatus("Ready to start");
}

void Blackjack_DrawStatus(void)
{
    uint16_t statusX = 0;
    uint16_t statusY = 250;
    uint16_t statusW = 240;
    uint16_t statusH = 20;

    LCD_Fill_Rectangle(statusX, statusY, statusW, statusH, LCD_COLOR_DARKGREEN);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(statusX + 5, statusY + 5, gBJ.statusText);
}

void Blackjack_DrawHands(void)
{
    char buf[24];

    // Clear dealer & player card areas with table color
    LCD_SetTextColor(LCD_COLOR_DARKGREEN);
    // Dealer area: y ~ 50–140
    LCD_Fill_Rectangle(0, 50, 240, 90, LCD_COLOR_DARKGREEN);
    // Player area: y ~ 150–230
    LCD_Fill_Rectangle(0, 150, 240, 80, LCD_COLOR_DARKGREEN);

    //
    // Dealer cards
    //
    uint16_t x = 20;
    uint16_t y = 60;

    for (uint8_t i = 0; i < gBJ.dealerCardCount; i++) {
        // Hide dealer's second card while it's the player's turn
        if ((i == 1U) && (gBJ.state == BJ_STATE_PLAYER_TURN)) {
            uint16_t cardW = 40;
            uint16_t cardH = 60;

            LCD_Fill_Rectangle(x, y, cardW, cardH, LCD_COLOR_LIGHTGRAY);
            LCD_Draw_Rectangle(x, y, cardW, cardH, LCD_COLOR_BLACK);

            LCD_SetTextColor(LCD_COLOR_BLACK);
            LCD_Draw_String(x + 6, y + 20, "??");
        } else {
            Blackjack_DrawCardOnScreen(gBJ.dealerHand[i], x, y);
        }

        x += 45;  // spacing between dealer cards
    }

    //
    // Player cards
    //
    x = 20;
    y = 170;

    for (uint8_t i = 0; i < gBJ.playerCardCount; i++) {
        Blackjack_DrawCardOnScreen(gBJ.playerHand[i], x, y);
        x += 45;  // spacing between player cards
    }

    //
    // Totals + status
    //
    LCD_SetTextColor(LCD_COLOR_WHITE);

    // Dealer total
    if (gBJ.state == BJ_STATE_PLAYER_TURN) {
        // Hide dealer total until player stands
        LCD_Draw_String(0, 125, "Dealer: ??");
    } else {
        snprintf(buf, sizeof(buf), "Dealer: %d", (int)gBJ.dealerTotal);
        LCD_Draw_String(0, 125, buf);
    }

    snprintf(buf, sizeof(buf), "Player: %d", (int)gBJ.playerTotal);
    LCD_Fill_Rectangle(0, 235, 240, 20, LCD_COLOR_DARKGREEN);
    LCD_Draw_String(0, 235, buf);

    Blackjack_DrawStatus();
}

// ---------------------
//  Cheats
// ---------------------

static uint8_t BJ_IsTenValue(CardRank rank)
{
    switch (rank) {
    case RANK_JACK:
    case RANK_QUEEN:
    case RANK_KING:
        return 1u;
    default:
        return (rank >= RANK_TEN) ? 1u : 0u;
    }
}

static void BJ_ForceNextPlayerBlackjack(void)
{
    uint8_t i;

    // Find an Ace from current deckIndex forward
    int8_t aceIndex = -1;
    for (i = gBJ.deckIndex; i < 52U; ++i) {
        if (gBJ.deck[i].rank == RANK_ACE) {
            aceIndex = (int8_t)i;
            break;
        }
    }

    // Put Ace at deckIndex (P1)
    if ((aceIndex >= 0) && (aceIndex != (int8_t)gBJ.deckIndex)) {
        Card tmp = gBJ.deck[gBJ.deckIndex];
        gBJ.deck[gBJ.deckIndex] = gBJ.deck[aceIndex];
        gBJ.deck[aceIndex]      = tmp;
    }

    // Find a 10-value card for P2 at deckIndex+2
    uint8_t start    = gBJ.deckIndex + 1U;
    int8_t  tenIndex = -1;

    for (i = start; i < 52U; ++i) {
        if (BJ_IsTenValue(gBJ.deck[i].rank)) {
            tenIndex = (int8_t)i;
            break;
        }
    }

    if (tenIndex >= 0) {
        uint8_t playerSecondPos = gBJ.deckIndex + 2U;
        if ((playerSecondPos < 52U) &&
            (tenIndex != (int8_t)playerSecondPos)) {
            Card tmp = gBJ.deck[playerSecondPos];
            gBJ.deck[playerSecondPos] = gBJ.deck[tenIndex];
            gBJ.deck[tenIndex]        = tmp;
        }
    }
}

// ---------------------
//  Helpers / init
// ---------------------

void Blackjack_InitGame(void)
{
    // Seed RNG once (okay to reseed; this is simple lab code)
    srand(HAL_GetTick());

    BJ_BuildDeck();
    BJ_ShuffleDeck();

    gBJ.playerCardCount = 0;
    gBJ.dealerCardCount = 0;
    gBJ.playerTotal     = 0;
    gBJ.dealerTotal     = 0;
    gBJ.state           = BJ_STATE_IDLE;

    BJ_SetStatus("Welcome to Blackjack");
}

// Draw table and buttons
void Blackjack_DrawTable(void)
{
    // Table background
    LCD_Clear(LCD_COLOR_DARKGREEN);

    // Title
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_Draw_String(20, 20, "Blackjack");

    // Divider line between dealer and player
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_Fill_Rectangle(0, 145, 240, 2, LCD_COLOR_BLACK);

    // Buttons: Hit, Stand, New
    LCD_Draw_Button(
        sBtnHit.x, sBtnHit.y, sBtnHit.w, sBtnHit.h,
        LCD_COLOR_LIGHTGREEN, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Hit");

    LCD_Draw_Button(
        sBtnStand.x, sBtnStand.y, sBtnStand.w, sBtnStand.h,
        LCD_COLOR_YELLOW, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "Stand");

    LCD_Draw_Button(
        sBtnNew.x, sBtnNew.y, sBtnNew.w, sBtnNew.h,
        LCD_COLOR_LIGHTGRAY, LCD_COLOR_BLACK, LCD_COLOR_BLACK,
        "New");
}

void Blackjack_StartNewRound(void)
{
    // Re-shuffle if deck is getting low
    if ((52U - gBJ.deckIndex) < 10U) {
        BJ_BuildDeck();
        BJ_ShuffleDeck();
    }

    // If cheat code is armed, rig next hand as blackjack for player
    if (CheatCode_IsArmed()) {
        BJ_ForceNextPlayerBlackjack();
        CheatCode_Consume();
    }

    gBJ.playerCardCount = 0;
    gBJ.dealerCardCount = 0;

    // Initial deal: P, D, P, D
    gBJ.playerHand[gBJ.playerCardCount++] = BJ_DealCard();
    gBJ.dealerHand[gBJ.dealerCardCount++] = BJ_DealCard();
    gBJ.playerHand[gBJ.playerCardCount++] = BJ_DealCard();
    gBJ.dealerHand[gBJ.dealerCardCount++] = BJ_DealCard();

    gBJ.playerTotal = BJ_HandValue(gBJ.playerHand, gBJ.playerCardCount);
    gBJ.dealerTotal = BJ_HandValue(gBJ.dealerHand, gBJ.dealerCardCount);

    gBJ.state = BJ_STATE_PLAYER_TURN;
    BJ_SetStatus("Your turn: Hit/Stand");
}

// ---------------------
//  Player actions
// ---------------------

void Blackjack_PlayerHit(void)
{
    if (gBJ.state != BJ_STATE_PLAYER_TURN) {
        return;
    }

    if (gBJ.playerCardCount >= BLACKJACK_MAX_HAND_CARDS) {
        BJ_SetStatus("Max cards reached");
        Blackjack_DrawHands();
        return;
    }

    gBJ.playerHand[gBJ.playerCardCount++] = BJ_DealCard();
    gBJ.playerTotal = BJ_HandValue(gBJ.playerHand, gBJ.playerCardCount);

    if (gBJ.playerTotal > 21) {
        BJ_SetStatus("Bust! Dealer wins.");
        Bank_ApplyLoss();
        gBJ.state = BJ_STATE_ROUND_DONE;
    } else if (gBJ.playerTotal == 21) {
        BJ_SetStatus("21! Standing.");
        Bank_ApplyPush();
        gBJ.state = BJ_STATE_DEALER_TURN;
    } else {
        BJ_SetStatus("Your turn: Hit/Stand");
    }

    Blackjack_DrawHands();

    if (gBJ.state == BJ_STATE_DEALER_TURN) {
        // Reuse dealer logic
        Blackjack_PlayerStand();
    }
}

void Blackjack_PlayerStand(void)
{
    if ((gBJ.state != BJ_STATE_PLAYER_TURN) &&
        (gBJ.state != BJ_STATE_DEALER_TURN)) {
        return;
    }

    gBJ.state = BJ_STATE_DEALER_TURN;

    // Dealer hits until 17 or more
    while ((gBJ.dealerTotal < 17) &&
           (gBJ.dealerCardCount < BLACKJACK_MAX_HAND_CARDS)) {
        gBJ.dealerHand[gBJ.dealerCardCount++] = BJ_DealCard();
        gBJ.dealerTotal = BJ_HandValue(gBJ.dealerHand, gBJ.dealerCardCount);
    }

    // Decide outcome
    if (gBJ.dealerTotal > 21) {
        BJ_SetStatus("Dealer bust! You win");
        Bank_ApplyWin();
    } else if (gBJ.dealerTotal > gBJ.playerTotal) {
        BJ_SetStatus("Dealer wins.");
        Bank_ApplyLoss();
    } else if (gBJ.dealerTotal < gBJ.playerTotal) {
        BJ_SetStatus("You win!");
        Bank_ApplyWin();
    } else if ((gBJ.dealerTotal < gBJ.playerTotal) &&
               (gBJ.playerTotal == 21)) {
        // Note: order here matches original logic
        BJ_SetStatus("Blackjack! You win!");
        Bank_ApplyBlackjackWin();
    } else {
        BJ_SetStatus("Push (tie).");
        Bank_ApplyPush();
    }

    gBJ.state = BJ_STATE_ROUND_DONE;
    Blackjack_DrawHands();
}

// ---------------------
//  Touch handling
// ---------------------

void Blackjack_HandleTouch(uint16_t x, uint16_t y)
{
    // Hit / Stand / New are always active; state logic
    // inside the functions decides if they do anything.
    if (BJ_ButtonHit(&sBtnHit, x, y)) {
        Blackjack_PlayerHit();
    } else if (BJ_ButtonHit(&sBtnStand, x, y)) {
        Blackjack_PlayerStand();
    } else if (BJ_ButtonHit(&sBtnNew, x, y)) {

        if (!Bank_CanBet()) {
            BJ_SetStatus("Balance < $100");
            Blackjack_DrawHands();
            return;
        }

        // Start a fresh round: reset/deal/update state
        Blackjack_StartNewRound();

        // Redraw table + Hit/Stand/New buttons
        Blackjack_DrawTable();

        // Redraw dealer + player cards + status
        Blackjack_DrawHands();

        // Redraw Exit button that GameDriver originally drew
        LCD_Draw_Button(
            180, 0, 60, 40,
            LCD_COLOR_LIGHTGRAY,
            LCD_COLOR_BLACK,
            LCD_COLOR_BLACK,
            "Exit");
    }
}
