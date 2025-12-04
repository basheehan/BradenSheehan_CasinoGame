#include "Bank.h"

static int32_t sBankBalance = BANK_STARTING_BALANCE;

void Bank_Init(void)
{
    sBankBalance = BANK_STARTING_BALANCE;
}

int32_t Bank_GetBalance(void)
{
    return sBankBalance;
}

uint8_t Bank_CanBet(void)
{
    return (sBankBalance >= BANK_BET_AMOUNT) ? 1u : 0u;
}

void Bank_ApplyWin(void)
{
    sBankBalance += BANK_BET_AMOUNT;
}
void Bank_ApplyBlackjackWin(void)
{
    sBankBalance += (BANK_BET_AMOUNT * 1.5);
}

void Bank_ApplyLoss(void)
{
    sBankBalance -= BANK_BET_AMOUNT;

    if (sBankBalance < 0)
    {
        sBankBalance = 0;
    }
}

void Bank_ApplyPush(void)
{
    // No change but add for function structure
}

void Bank_ApplyRouletteColorWin(void)
{
    sBankBalance += 50;
}
void Bank_ApplyRouletteNumberWin(void)
{
    sBankBalance += (BANK_BET_AMOUNT + 200);
}

