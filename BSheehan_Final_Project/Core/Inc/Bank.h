#ifndef BANK_H
#define BANK_H

#include <stdint.h>

#define BANK_STARTING_BALANCE 1000
#define BANK_BET_AMOUNT       100

void Bank_Init(void);
int32_t Bank_GetBalance(void);
uint8_t Bank_CanBet(void);

void Bank_ApplyWin(void);
void Bank_ApplyLoss(void);
void Bank_ApplyPush(void);
void Bank_ApplyBlackjackWin(void);

#endif /* BANK_H */
