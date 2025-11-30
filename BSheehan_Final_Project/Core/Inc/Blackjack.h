#ifndef BLACKJACK_H_
#define BLACKJACK_H_

#include <stdlib.h>  
#include <stdint.h>     
#include "stm32f4xx_hal.h"


// Suit and rank definitions
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

// Card helpers / API
uint8_t Blackjack_DrawCardId(void);
Card Blackjack_DrawCard(void);
Card Blackjack_CardFromId(uint8_t id);

// Draw a card at (x,y) on the screen
void Blackjack_DrawCardOnScreen(Card card, uint16_t x, uint16_t y);

#endif