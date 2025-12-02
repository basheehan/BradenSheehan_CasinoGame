#include "CheatCode.h"

// >= 1 second first hold
#define CHEAT_MIN_HOLD1_MS 1000U
// >= 2 second second hold
#define CHEAT_MIN_HOLD2_MS 2000U
// "Quick tap" max duration
#define CHEAT_MAX_TAP_MS 500U
// If user pauses too long between steps, reset
#define CHEAT_STEP_TIMEOUT_MS 5000U

static uint8_t sCheatStep = 0U;
static uint8_t sLastButtonPressed = 0U;
static uint32_t sPressStartMs = 0U;
static uint32_t sLastStepChangeMs = 0U;

static bool sCheatArmed = false;

void CheatCode_Init(void)
{
    sCheatStep = 0U;
    sLastButtonPressed = 0U;
    sPressStartMs = 0U;
    sLastStepChangeMs = 0U;
    sCheatArmed = false;

    // init da button

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // pressed = high on PA0
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    printf("button init ed __________________");
}

static void CheatCode_ResetSequence(uint32_t nowMs)
{
    sCheatStep = 0U;
    sLastStepChangeMs = nowMs;
}

void CheatCode_Update(uint8_t buttonPressed, uint32_t nowMs)
{
    // Timeout between steps
    if (sCheatStep != 0U)
    {
        if ((nowMs - sLastStepChangeMs) > CHEAT_STEP_TIMEOUT_MS)
        {
            CheatCode_ResetSequence(nowMs);
        }
    }

    // Rising edge
    if (buttonPressed && !sLastButtonPressed)
    {
        printf("CHEAT BTN: pressed at %lu ms\r\n", (unsigned long)nowMs);

        sPressStartMs = nowMs;
    }

    // Falling edge
    if (!buttonPressed && sLastButtonPressed)
    {
        uint32_t heldMs = nowMs - sPressStartMs;

        switch (sCheatStep)
        {
        case 0U:
            // First long hold (>= 1s)
            if (heldMs >= CHEAT_MIN_HOLD1_MS)
            {
                sCheatStep = 1U;
                sLastStepChangeMs = nowMs;
            }
            else
            {
                CheatCode_ResetSequence(nowMs);
            }
            break;

        case 1U:
            // Second long hold (>= 2s)
            if (heldMs >= CHEAT_MIN_HOLD2_MS)
            {
                sCheatStep = 2U;
                sLastStepChangeMs = nowMs;
            }
            else
            {
                CheatCode_ResetSequence(nowMs);
            }
            break;

        case 2U:
            // Quick tap (<= 0.5s)
            if (heldMs <= CHEAT_MAX_TAP_MS)
            {
                sCheatArmed = true;
                printf("CHEAT CODE ACTIVATED at %lu ms\r\n", (unsigned long)nowMs);
            }
            CheatCode_ResetSequence(nowMs);
            break;

        default:
            CheatCode_ResetSequence(nowMs);
            break;
        }
    }

    sLastButtonPressed = buttonPressed ? 1U : 0U;
}

bool CheatCode_IsArmed(void)
{
    return sCheatArmed;
}

void CheatCode_Consume(void)
{
    sCheatArmed = false;
}

bool CheatCode_IsSequenceInProgress(void)
{
    return (sCheatStep != 0U);
}
