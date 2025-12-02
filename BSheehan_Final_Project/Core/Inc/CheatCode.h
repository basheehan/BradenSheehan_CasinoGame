#ifndef CHEATCODE_H
#define CHEATCODE_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

void CheatCode_Init(void);
void CheatCode_Update(uint8_t buttonPressed, uint32_t nowMs);

bool CheatCode_IsArmed(void);
void CheatCode_Consume(void);

bool CheatCode_IsSequenceInProgress(void);

#endif /* CHEATCODE_H */
