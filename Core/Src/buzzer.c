/*
 * buzzer.c
 */

#include "buzzer.h"
#include "main.h"

// handler do timer
TIM_HandleTypeDef htim3;

// -------------------------
// INIT PWM
// -------------------------
void Buzzer_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PB1 = TIM3_CH4
    GPIO_InitStruct.Pin = BUZZER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;

    HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

    // Timer config
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 84 - 1; // 84MHz / 84 = 1 MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;  // valor inicial
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_PWM_Init(&htim3);

    TIM_OC_InitTypeDef sConfigOC = {0};

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500; // duty 50%
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);
}

// -------------------------
// FUNCAO BASE (PWM)
// -------------------------
static void Buzzer_Tone(uint16_t freq, uint16_t time_ms)
{
    uint32_t timer_clock = 1000000; // 1 MHz

    uint32_t period = (timer_clock / freq) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, period / 2);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    HAL_Delay(time_ms);

    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
}

// -------------------------
// SONS DO JOGO
// -------------------------

// som estilo arcade para menu (loop ou chamada unica)
// ----------------------------------------------------------------------
// FUNÇÃO AUXILIAR DE DISPARO RÁPIDO
// Liga a frequência e não trava a CPU com HAL_Delay
// ----------------------------------------------------------------------
void Buzzer_Tone_NoDelay(uint16_t freq)
{
    if (freq == 0) {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
        return;
    }
    uint32_t timer_clock = 1000000; // 1 MHz
    uint32_t period = (timer_clock / freq) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period);

    // TRUQUE DE ENERGIA: Em vez de period / 2 (50% de consumo),
    // usamos period / 6 (~16% de consumo). O buzzer consome MUITO menos corrente!
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, period / 6);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
}

// ----------------------------------------------------------------------
// NOVA FUNÇÃO DO MENU (Apenas define a nota com base num índice)
// ----------------------------------------------------------------------
void Buzzer_Menu_Step(int nota_atual)
{
    switch(nota_atual) {
        case 0: Buzzer_Tone_NoDelay(330); break; // Mi médio (E4) - Confortável e limpo
        case 1: Buzzer_Tone_NoDelay(294); break; // Ré médio (D4)
        case 2: Buzzer_Tone_NoDelay(247); break; // Si grave (B3)
        case 3: Buzzer_Tone_NoDelay(262); break; // Dó médio (C4)
        default: Buzzer_Tone_NoDelay(0);   break; // Silêncio de segurança
    }
}

// tiro (agudo curto)
void Buzzer_Shoot(void)
{
    for (int freq = 2500; freq > 1500; freq -= 200)
    {
        Buzzer_Tone(freq, 10);
    }
}


// acerto (descida de tom)
void Buzzer_Hit(void)
{
    Buzzer_Tone(1400, 50);
    HAL_Delay(20);
    Buzzer_Tone(900, 60);
}

// inicio do jogo
void Buzzer_Start(void)
{
    Buzzer_Tone(800, 80);
    HAL_Delay(30);

    Buzzer_Tone(1200, 80);
    HAL_Delay(30);

    Buzzer_Tone(1800, 120);
}

// game over (descendente)
void Buzzer_GameOver(void)
{
    // Notas decrescentes pesadas e melancólicas
    Buzzer_Tone(392, 150); // Sol (G4)
    HAL_Delay(40);
    Buzzer_Tone(349, 150); // Fá (F4)
    HAL_Delay(40);
    Buzzer_Tone(311, 150); // Ré# (Eb4)
    HAL_Delay(40);

    // Arpejo rápido descendente (Efeito colapso de sistema)
    for (int freq = 300; freq > 150; freq -= 25)
    {
        Buzzer_Tone(freq, 25);
    }

    Buzzer_Tone(130, 400); // Nota ultra grave final
}



