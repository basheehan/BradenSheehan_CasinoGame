#ifndef BLACKJACK_H_
#define BLACKJACK_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "LCD_Driver.h"

// -------- Card definitions --------

typedef enum {
    SUIT_CLUBS = 0,
    SUIT_DIAMONDS,
    SUIT_HEARTS,
    SUIT_SPADES,
    SUIT_COUNT
} CardSuit;

typedef enum {
    RANK_ACE = 1,
    RANK_TWO,
    RANK_THREE,
    RANK_FOUR,
    RANK_FIVE,
    RANK_SIX,
    RANK_SEVEN,
    RANK_EIGHT,
    RANK_NINE,
    RANK_TEN,
    RANK_JACK,
    RANK_QUEEN,
    RANK_KING
} CardRank;

typedef struct {
    CardRank rank;
    CardSuit suit;
} Card;

// -------- Game state --------

#define BLACKJACK_MAX_HAND_CARDS   12U


typedef enum {
    BJ_STATE_IDLE = 0,
    BJ_STATE_PLAYER_TURN,
    BJ_STATE_DEALER_TURN,
    BJ_STATE_ROUND_DONE
} BlackjackState_t;

typedef struct {
    Card    deck[52];
    uint8_t deckIndex;

    Card    playerHand[BLACKJACK_MAX_HAND_CARDS];
    uint8_t playerCardCount;
    int16_t playerTotal;

    Card    dealerHand[BLACKJACK_MAX_HAND_CARDS];
    uint8_t dealerCardCount;
    int16_t dealerTotal;

    BlackjackState_t state;
    char    statusText[32];
} BlackjackGame_t;

// -------- Public API --------

// Core game lifecycle
void Blackjack_InitGame(void);
void Blackjack_DrawTable(void);
void Blackjack_StartNewRound(void);

// Player actions
void Blackjack_PlayerHit(void);
void Blackjack_PlayerStand(void);

// Drawing helpers
void Blackjack_DrawHands(void);
void Blackjack_DrawStatus(void);

// Optional: simple touch handler for Hit / Stand / New buttons
// (Call this from GameDriver when in APP_STATE_BLACKJACK)
void Blackjack_HandleTouch(uint16_t x, uint16_t y);

// Card helpers (useful if you want to draw individual cards elsewhere)
Card Blackjack_CardFromId(uint8_t id);
void Blackjack_DrawCardOnScreen(Card card, uint16_t x, uint16_t y);

void Blackjack_StartRound(void);
void Blackjack_Reset(void);



#endif /* BLACKJACK_H_ */
