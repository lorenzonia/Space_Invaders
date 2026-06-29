/*
 * buzzer.h
 */

#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "main.h"

// Funções originais do seu projeto
void Buzzer_Init(void);
void Buzzer_Shoot(void);
void Buzzer_Hit(void);
void Buzzer_GameOver(void);
void Buzzer_Start(void);

// =======================================================================
// NOVAS DECLARAÇÕES PARA A MÚSICA DO MENU EM RTOS (ADICIONE ESTAS LINHAS)
// =======================================================================
void Buzzer_Tone_NoDelay(uint16_t freq);
void Buzzer_Menu_Step(int nota_atual);

#endif /* INC_BUZZER_H_ */
