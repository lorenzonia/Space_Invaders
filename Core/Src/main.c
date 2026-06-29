//USER CODE BEGIN Header
//*
//******************************************************************************
//* @file : main.c
//* @brief : Main program body
//******************************************************************************
//* @attention
//*
//* Copyright (c) 2024 STMicroelectronics.
//* All rights reserved.
//*
//* This software is licensed under terms that can be found in the LICENSE file
//* in the root directory of this software component.
//* If no LICENSE file comes with this software, it is provided AS-IS.
//*
//******************************************************************************

//USER CODE END Header
//Includes ------------------------------------------------------------------
#include "main.h"
#include "cmsis_os.h"

//Private includes ----------------------------------------------------------
//USER CODE BEGIN Includes
#include <string.h>
#include <stdlib.h>
#include "stdio.h"
#include "atraso.h"
#include "defPrincipais.h"
#include "PRNG_LFSR.h"
#include "st7735.h"
#include "game.h"
#include "figuras.h"
#include "game_config.h"
#include "enemy.h"
#include "buzzer.h"
#include "semphr.h"

//USER CODE END Includes

//Private typedef -----------------------------------------------------------
//USER CODE BEGIN PTD

//USER CODE END PTD

//Private define ------------------------------------------------------------
//USER CODE BEGIN PD

//USER CODE END PD

//Private macro -------------------------------------------------------------
//USER CODE BEGIN PM

///USER CODE END PM

//Private variables ---------------------------------------------------------
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

SPI_HandleTypeDef hspi1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = { .name = "defaultTask",
		.stack_size = 128 * 4, .priority = (osPriority_t) osPriorityNormal, };
//USER CODE BEGIN PV

//Criando o objeto global do jogo
Game game;
Game game_render_copy;

uint16_t ADC_buffer[2];
uint16_t valor_ADC[2];

// Objetos do CMSIS-RTOS V2
osSemaphoreId_t renderSemaphoreHandle;
osSemaphoreId_t physicsSemaphoreHandle;
osMutexId_t gameMutexHandle;

//USER CODE END PV

//Private function prototypes -----------------------------------------------
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC1_Init(void);
void StartDefaultTask(void *argument);

//USER CODE BEGIN PFP

//USER CODE END PFP

//Private user code ---------------------------------------------------------
//USER CODE BEGIN 0

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {

	valor_ADC[0] = ADC_buffer[0];
	valor_ADC[1] = ADC_buffer[1];
}

//---------------------------------------------------------------------------------------------------
//TASKS DO JOGO PARA O PROJETO

void vTask_Input(void *pvParameters) {
	Game *game = (Game*) pvParameters;

	int cooldown = 0;

	static int last_botao = 0;   // detecta borda

	while (1) {
		uint16_t x = ADC_buffer[0];
		int botao = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);

		// -------------------------
		// DETECCAO DE BORDA (ANTI-REPETICAO)
		// -------------------------
		int botao_pressed = (botao && !last_botao);
		last_botao = botao;

		// -------------------------
		// MENU
		// -------------------------
		if (game->state == MENU) {
			if (botao_pressed) {
				srand(osKernelGetTickCount());
				Game_Reset(game);

				game->state = PLAYING;

				// som de inicio
				Buzzer_Start();
			}
		}

		// -------------------------
		// PLAYING
		// -------------------------
		else if (game->state == PLAYING) {
			// movimento horizontal
			if (x < 1000)
				game->player.x -= game->player.speed;
			else if (x > 3000)
				game->player.x += game->player.speed;

			// limites
			if (game->player.x < 0)
				game->player.x = 0;

			if (game->player.x > (160 - PLAYER_WIDTH))
				game->player.x = 160 - PLAYER_WIDTH;

			// tiro
			if (botao_pressed && cooldown == 0) {
				Bullet_Fire(game->bullets, game->player.x + (PLAYER_WIDTH / 2),
						game->player.y - 2);

				// SOM DE TIRO
				Buzzer_Shoot();

				cooldown = 5;
			}

			if (cooldown > 0)
				cooldown--;
		}

		// -------------------------
		// GAME OVER
		// -------------------------
		else if (game->state == GAME_OVER) {
			if (botao_pressed) {
				Game_Reset(game);
				game->state = MENU;
			}
		}

		// controle de taxa da input
		vTaskDelay(15);
	}
}

//---------------------------------------------------------------------------------------------------

void vTask_Game(void *pvParameters) {
	Game *game = (Game*) pvParameters;

	static GameState last_state = -1;   // Detecta mudança de estado
	static int last_level_score = 0; // >>> TRAVA: Impede o loop infinito nos 100 pontos <<<
	int tocar_som_hit = 0;
	int jogo_inicializado = 0;

	// LIMPEZA ABSOLUTA DE ENERGIZAÇÃO (Zera lixo de memória RAM no Cold Start)
	game->score = 0;
	game->level = 1;
	for (int i = 0; i < MAX_ENEMIES; i++) {
		game->enemies[i].active = 0;
		game->enemies[i].explosion_timer = 0;
		game->enemies[i].x = 0.0f;
		game->enemies[i].y = 0.0f;
		game->enemies[i].last_x = 0.0f;
		game->enemies[i].last_y = 0.0f;
	}

	while (1) {
		// Handshake obrigatório com a vTask_Render
		osSemaphoreAcquire(physicsSemaphoreHandle, osWaitForever);

		// -------------------------------------------------------------
		// MÁQUINA DE ESTADOS DA FÍSICA (DETECTA TRANSIÇÕES)
		// -------------------------------------------------------------
		if (game->state != last_state) {
			if (game->state == MENU) {
				jogo_inicializado = 0;
			} else if (game->state == PLAYING) {
				Buzzer_Start();
				game->score = 0;
				game->level = 1;
				last_level_score = 0; // Reseta a trava de pontuação do nível

				// Força limpeza completa das structs de inimigos para a primeira partida
				for (int i = 0; i < MAX_ENEMIES; i++) {
					game->enemies[i].active = 0;
					game->enemies[i].explosion_timer = 0;
					game->enemies[i].x = 0.0f;
					game->enemies[i].y = (float) GAME_TOP;
					game->enemies[i].last_x = 0.0f;
					game->enemies[i].last_y = (float) GAME_TOP;
				}
				Enemy_Spawn(game->enemies, 3);
				jogo_inicializado = 1;
			} else if (game->state == GAME_OVER) {
				Buzzer_GameOver();
			}
			last_state = game->state;
		}

		// -------------------------------------------------------------
		// LOOP DE JOGO ATIVO (PLAYING)
		// -------------------------------------------------------------
		if (game->state == PLAYING) {

			// Guardar histórico do player
			game->player.last_x = game->player.x;
			game->player.last_y = game->player.y;

			// Velocidade dinâmica subindo de 0.5f em 0.5f por nível
			float velocidade = 1.0f + (game->level * 0.5f);

			// Movimento dos inimigos
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (game->enemies[i].active) {
					Enemy_Update(&game->enemies[i], velocidade);

					// Se passou do limite inferior, reseta de forma segura no topo
					if (game->enemies[i].y > GAME_BOTTOM) {
						game->enemies[i].y = (float) GAME_TOP;
						game->enemies[i].x = (float) (rand()
								% (SCREEN_WIDTH - ENEMY_WIDTH));
						game->enemies[i].last_x = game->enemies[i].x;
						game->enemies[i].last_y = game->enemies[i].y;
					}
				}
			}

			// Colisão Player x Inimigo
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (game->enemies[i].active) {
					if (abs((int) game->player.x - (int) game->enemies[i].x)
							< (PLAYER_WIDTH - 2)
							&& (game->enemies[i].y
									>= game->player.y - PLAYER_HITBOX_HEIGHT
									&& game->enemies[i].y
											<= game->player.y
													+ PLAYER_HITBOX_Y_OFFSET)) {
						game->state = GAME_OVER;
					}
				}
			}

			// Atualiza Tiros
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (game->bullets[i].active) {
					game->bullets[i].last_x = game->bullets[i].x;
					game->bullets[i].last_y = game->bullets[i].y;
					game->bullets[i].y -= 6;

					if (game->bullets[i].y < GAME_TOP) {
						game->bullets[i].active = 0;
					}
				}
			}

			// Colisão Tiro x Inimigo
			for (int i = 0; i < MAX_BULLETS; i++) {
				if (game->bullets[i].active) {
					for (int j = 0; j < MAX_ENEMIES; j++) {
						if (game->enemies[j].active) {
							if (abs(
									game->bullets[i].x
											- (int) game->enemies[j].x) < 8
									&& abs(
											game->bullets[i].y
													- (int) game->enemies[j].y)
											< 8) {

								game->bullets[i].active = 0;
								game->enemies[j].active = 0;
								game->enemies[j].explosion_timer = 5;
								game->score += 10;

								// Guarda e limpa IMEDIATAMENTE o rastro do sprite morto no ecrã
								int dead_x = (int) game->enemies[j].x;
								int dead_y = (int) game->enemies[j].y;
								game->enemies[j].last_x = game->enemies[j].x;
								game->enemies[j].last_y = game->enemies[j].y;

								int cx = dead_x - 2;
								int cy = dead_y - 4;
								if (cx < 0)
									cx = 0;
								if (cy < GAME_TOP)
									cy = GAME_TOP;
								ST7735_FillRectangle(cx, cy, ENEMY_WIDTH + 6,
								ENEMY_HEIGHT + 10, ST7735_BLACK);

								tocar_som_hit = 1;

								// Sobe de nível com trava de segurança de frame único
								if (game->score % 100 == 0
										&& game->score != last_level_score) {
									game->level++;
									last_level_score = game->score;
								}
							}
						}
					}
				}
			}

			// Atualiza Explosões
			for (int i = 0; i < MAX_ENEMIES; i++) {
				if (game->enemies[i].explosion_timer > 0) {
					game->enemies[i].explosion_timer--;
				}
			}

			// Manutenção de Fluxo constante de naves (Teto de 3 ativos)
			if (jogo_inicializado) {
				int ativos = contar_inimigos(game->enemies);
				if (ativos < 3) {
					Enemy_Spawn(game->enemies, 3 - ativos);
				}
			}
		}

		// Copia os dados estáveis para a renderização desenhar
		osMutexAcquire(gameMutexHandle, osWaitForever);
		memcpy(&game_render_copy, game, sizeof(Game));
		osMutexRelease(gameMutexHandle);

		if (tocar_som_hit) {
			Buzzer_Hit();
			tocar_som_hit = 0;
		}

		// Acorda a vTask_Render e segura o passo no ciclo estável do RTOS
		osSemaphoreRelease(renderSemaphoreHandle);
		vTaskDelay(pdMS_TO_TICKS(16));
	}
}

// ----------------------------------------------------------------------
// TASK DA MÚSICA DO MENU - ULTRA BAIXO CONSUMO (Flicker Zero)
// ----------------------------------------------------------------------
void vTask_SoundMenu(void *pvParameters) {
	Game *game = (Game*) pvParameters;
	int passo_nota = 0;

	while (1) {
		if (game->state == MENU) {

			if (passo_nota < 4) {
				Buzzer_Menu_Step(passo_nota);

				// Reduzido para 30ms: o bip vira um estalo nítido e economiza energia
				vTaskDelay(pdMS_TO_TICKS(30));

				Buzzer_Menu_Step(-1); // Corta totalmente a energia do pino

				// Aumentado para 90ms: tempo de sobra para a energia estabilizar
				vTaskDelay(pdMS_TO_TICKS(90));

				passo_nota++;
			}
			else {
				Buzzer_Menu_Step(-1);
				passo_nota = 0;

				// Espera 600ms em silêncio absoluto para o display respirar antes do loop
				vTaskDelay(pdMS_TO_TICKS(600));
			}
		}
		else {
			Buzzer_Menu_Step(-1);
			passo_nota = 0;
			vTaskDelay(pdMS_TO_TICKS(200));
		}
	}
}

//---------------------------------------------------------------------------------------------------

void vTask_Render(void *pvParameters) {
	Game *game = &game_render_copy;
	static GameState last_state = -1;

	while (1) {
		// Aguarda o sinal verde da vTask_Game por tempo indeterminado
		if (osSemaphoreAcquire(renderSemaphoreHandle, osWaitForever) == osOK) {
			// PROTEÇÃO MÁXIMA: Garante que a física não altere a cópia enquanto o LCD é atualizado
			osMutexAcquire(gameMutexHandle, osWaitForever);

			// =========================
			// LIMPA AO TROCAR ESTADO
			// =========================
			if (game->state != last_state) {
				ST7735_FillScreenFast(ST7735_BLACK);
				last_state = game->state;
			}

			// =======================================================================
			// ================= STATE: MENU (ESTILO GAME BOY CORRIGIDO) ==============
			// =======================================================================
			if (game->state == MENU) {
				// 1. MOLDURA EXTERNA (Simula a borda da tela fazendo 4 linhas com retângulos finos)
				// Linha Superior e Inferior
				ST7735_FillRectangle(2, 2, SCREEN_WIDTH - 4, 2, ST7735_GREEN);
				ST7735_FillRectangle(2, SCREEN_HEIGHT - 4, SCREEN_WIDTH - 4, 2,
				ST7735_GREEN);
				// Linha Esquerda e Direita
				ST7735_FillRectangle(2, 2, 2, SCREEN_HEIGHT - 4, ST7735_GREEN);
				ST7735_FillRectangle(SCREEN_WIDTH - 4, 2, 2, SCREEN_HEIGHT - 4,
				ST7735_GREEN);

				// 2. TÍTULO PRINCIPAL (Em destaque com fonte grande 11x18)
				ST7735_WriteString(25, 15, "SPACE", Font_11x18, ST7735_GREEN,
				ST7735_BLACK);
				ST7735_WriteString(45, 37, "INVADERS", Font_11x18, ST7735_RED,
				ST7735_BLACK);

				// 3. LINHA DIVISÓRIA ESTILIZADA (Estilo anos 80)
				ST7735_FillRectangle(15, 65, SCREEN_WIDTH - 30, 2,
				ST7735_YELLOW);

				// 4. INSTRUÇÃO PISCANTE/ESTÁTICA (Font_7x10)
				ST7735_WriteString(23, 80, "PRESSIONE O BOTAO", Font_7x10,
				ST7735_WHITE, ST7735_BLACK);
				ST7735_WriteString(33, 95, "PARA INICIAR", Font_7x10,
				ST7735_WHITE, ST7735_BLACK);

				// 5. CRÉDITOS DE RODAPÉ
				ST7735_WriteString(16, 146, "(C) 2026 LORENZONI", Font_7x10,
				ST7735_GREEN, ST7735_BLACK);
			}

			// =======================================================================
			// ================= STATE: PLAYING ======================================
			// =======================================================================
			else if (game->state == PLAYING) {
				// ================= HUD =================
				char str[20];
				sprintf(str, "Score:%03d", game->score);
				ST7735_WriteString(2, 0, str, Font_7x10, ST7735_GREEN,
				ST7735_BLACK);
				sprintf(str, "Lv:%d", game->level);
				ST7735_WriteString(110, 0, str, Font_7x10, ST7735_YELLOW,
				ST7735_BLACK);

				// ================= PLAYER =================
				int px = game->player.last_x - CLEAR_MARGIN_X;
				int py = game->player.last_y - CLEAR_MARGIN_Y;

				if (px < 0)
					px = 0;
				if (py < GAME_TOP)
					py = GAME_TOP;

				// Apaga a posição anterior da nave
				ST7735_FillRectangle(px, py, PLAYER_CLEAR_W, PLAYER_CLEAR_H,
				ST7735_BLACK);

				// Desenha a nave na posição nova
				ST7735_draw_figure(game->player.x, game->player.y - 1,
						nave_grande, ST7735_CYAN);

				// =======================================================================
				// --- INIMIGOS: RENDERIZAÇÃO INTELIGENTE ANTI-FLICKER (SINGLE PASS) ---
				// =======================================================================
				for (int i = 0; i < MAX_ENEMIES; i++) {
					int x_atual = (int) game->enemies[i].x;
					int y_atual = (int) game->enemies[i].y;
					int last_x_int = (int) game->enemies[i].last_x;
					int last_y_int = (int) game->enemies[i].last_y;

					if (game->enemies[i].active && y_atual < SCREEN_HEIGHT) {

					// SÓ APAGA SE ELE REALMENTE SE MOVEU
						if (x_atual != last_x_int || y_atual != last_y_int) {

							// Em vez de cobrir o inimigo todo com um retângulo preto,
							// limpamos apenas a "borda" ou rastro superior/lateral que sobrou.
							// Para simplificar e garantir estabilidade, limpamos a área antiga:
							ST7735_FillRectangle(last_x_int, last_y_int, ENEMY_WIDTH, ENEMY_HEIGHT, ST7735_BLACK);
						}

						// Desenha imediatamente por cima na posição nova (Reduz o tempo de pixel apagado)
						ST7735_FillRectangle(x_atual, y_atual, ENEMY_WIDTH, ENEMY_HEIGHT, ST7735_RED);

					} else if (game->enemies[i].explosion_timer > 0) {
						// Inimigo explodindo
						ST7735_FillRectangle(x_atual + 2, y_atual + 2, ENEMY_WIDTH - 4, ENEMY_HEIGHT - 4, ST7735_WHITE);
					}
				}

				// ================= TIROS (MUDOU PARA DENTRO DO IF PLAYING) =================
				for (int i = 0; i < MAX_BULLETS; i++) {
					int bx = game->bullets[i].last_x;
					int by = game->bullets[i].last_y;

					// Apaga a posição anterior do tiro
					if (by >= GAME_TOP && by < SCREEN_HEIGHT) {
						ST7735_FillRectangle(bx, by, 2, 10, ST7735_BLACK);
					}

					// Desenha o tiro na posição nova
					if (game->bullets[i].active
							&& game->bullets[i].y < SCREEN_HEIGHT) {
						ST7735_FillRectangle(game->bullets[i].x,
								game->bullets[i].y, 2, 4, ST7735_YELLOW);
					}
				}

				// ================= LIMPEZA EXTRA DO FUNDO (MUDOU PARA DENTRO DO IF PLAYING) =================
				ST7735_FillRectangle(0, SCREEN_HEIGHT - ENEMY_CLEAR_SIZE - 2,
				SCREEN_WIDTH, ENEMY_CLEAR_SIZE + 2, ST7735_BLACK);
			}

			// =======================================================================
			// ================= STATE: GAME OVER (ESTILO GAME BOY PADRONIZADO) ======
			// =======================================================================
			else if (game->state == GAME_OVER) {
				// 1. MOLDURA EXTERNA VERMELHA (Mesmo design do menu, mudando a cor para alerta)
				// Linha Superior e Inferior
				ST7735_FillRectangle(2, 2, SCREEN_WIDTH - 4, 2, ST7735_RED);
				ST7735_FillRectangle(2, SCREEN_HEIGHT - 4, SCREEN_WIDTH - 4, 2,
				ST7735_RED);
				// Linha Esquerda e Direita
				ST7735_FillRectangle(2, 2, 2, SCREEN_HEIGHT - 4, ST7735_RED);
				ST7735_FillRectangle(SCREEN_WIDTH - 4, 2, 2, SCREEN_HEIGHT - 4,
				ST7735_RED);

				// 2. TÍTULO PRINCIPAL (Fonte grande 11x18, centralizada e no mesmo Y do menu)
				// "GAME" tem 4 letras * 11px = 44px. (128-44)/2 = 42
				ST7735_WriteString(27, 15, "GAME OVER", Font_11x18, ST7735_RED,
				ST7735_BLACK);

				// 3. LINHA DIVISÓRIA ESTILIZADA (Exatamente igual ao Menu para manter o padrão)
				ST7735_FillRectangle(15, 40, SCREEN_WIDTH - 30, 2,
				ST7735_YELLOW);

				// 4. EXIBIÇÃO DO PLACAR FINAL (Centralizado no meio da tela)
				// Cria o buffer para ler o score real obtido
				char buffer_score[20];
				sprintf(buffer_score, "SCORE: %03d", game->score);
				// "SCORE: 000" tem 10 caracteres * 7px = 70px. (128-70)/2 = 29
				ST7735_WriteString(40, 55, buffer_score, Font_7x10,
				ST7735_GREEN, ST7735_BLACK);

				// 5. INSTRUÇÃO PARA REINICIAR (Mesma janela Y do menu)
				// "Pressione botao" -> 16 letras * 7px = 112px. (128-112)/2 = 8
				ST7735_WriteString(23, 80, "PRESSIONE O BOTAO", Font_7x10,
				ST7735_WHITE, ST7735_BLACK);
				// "Para reiniciar" -> 14 letras * 7px = 98px. (128-98)/2 = 15
				ST7735_WriteString(33, 95, "PARA REINICIAR", Font_7x10,
				ST7735_WHITE, ST7735_BLACK);

				// 6. CRÉDITOS DE RODAPÉ (Igual ao menu para selar a assinatura do jogo)
				ST7735_WriteString(16, 146, "(C) 2026 LORENZONI", Font_7x10,
				ST7735_RED, ST7735_BLACK);
			}

			// FIM DO BLOCO DE DESENHO: Libera o Mutex antes de liberar a física!
			osMutexRelease(gameMutexHandle);

			// Libera a física para calcular o próximo passo com segurança!
			osSemaphoreRelease(physicsSemaphoreHandle);
		}
	}
}

//---------------------------------------------------------------------------------------------------
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

/* USER CODE BEGIN 1 */
// uint32_t semente_PRNG=1; codigo original do professor
/* USER CODE END 1 */

/* MCU Configuration--------------------------------------------------------*/

/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
HAL_Init();

/* USER CODE BEGIN Init */

/* USER CODE END Init */

/* Configure the system clock */
SystemClock_Config();

/* USER CODE BEGIN SysInit */

/* USER CODE END SysInit */

/* Initialize all configured peripherals */
MX_GPIO_Init();
MX_DMA_Init();
MX_SPI1_Init();
MX_ADC1_Init();
Buzzer_Init();

HAL_ADC_Start_DMA(&hadc1, (uint32_t*) ADC_buffer, 2);

/* USER CODE BEGIN 2 */

//Inicialização do game
Game_Init(&game);

// inicializa LCD
HAL_Delay(100);
ST7735_Init();

ST7735_FillScreen(ST7735_BLACK);

HAL_ADC_Start_DMA(&hadc1, (uint32_t*) ADC_buffer, 2);

/* USER CODE END 2 */

/* Init scheduler */
osKernelInitialize();

/* USER CODE BEGIN RTOS_MUTEX */
// Atributos padrão para o Mutex
const osMutexAttr_t gameMutex_attributes = { .name = "gameMutex" };

// Cria o Mutex
gameMutexHandle = osMutexNew(&gameMutex_attributes);

/* USER CODE END RTOS_MUTEX */

/* USER CODE BEGIN RTOS_SEMAPHORES */
renderSemaphoreHandle = osSemaphoreNew(1, 1, NULL);
physicsSemaphoreHandle = osSemaphoreNew(1, 1, NULL); // <--- INICIALIZA COM 1 TOKEN
if (renderSemaphoreHandle == NULL || physicsSemaphoreHandle == NULL) {
	Error_Handler();
}
/* USER CODE END RTOS_SEMAPHORES */

/* USER CODE BEGIN RTOS_TIMERS */
/* start timers, add new ones, ... */
/* USER CODE END RTOS_TIMERS */

/* USER CODE BEGIN RTOS_QUEUES */
/* add queues, ... */
/* USER CODE END RTOS_QUEUES */

/* Create the thread(s) */
/* creation of defaultTask */
defaultTaskHandle = osThreadNew(StartDefaultTask, NULL,
		&defaultTask_attributes);

/* USER CODE BEGIN RTOS_THREADS */
/* add threads, ... */

// Definição de atributos e tamanho de Stack seguro para cada Task
const osThreadAttr_t input_attributes = { .name = "vTask_Input", .stack_size =
		256 * 4, .priority = osPriorityNormal };
const osThreadAttr_t game_attributes = { .name = "vTask_Game", .stack_size = 256
		* 4, .priority = osPriorityNormal };
// A Render ganha 512 * 4 (2048 bytes) para suportar o sprintf com segurança
const osThreadAttr_t render_attributes = { .name = "vTask_Render", .stack_size =
		512 * 4, .priority = osPriorityAboveNormal };
// ADICIONE OS ATRIBUTOS DA NOVA TASK DE SOM:
const osThreadAttr_t sound_attributes = { .name = "vTask_SoundMenu", .stack_size =
		256 * 4, .priority = osPriorityNormal };

// Criação das Tasks utilizando os atributos seguros
osThreadNew(vTask_Input, &game, &input_attributes);
osThreadNew(vTask_Game, &game, &game_attributes);
osThreadNew(vTask_Render, &game, &render_attributes);
osThreadNew(vTask_SoundMenu, &game, &sound_attributes);

/* USER CODE END RTOS_THREADS */

/* USER CODE BEGIN RTOS_EVENTS */
/* add events, ... */
/* USER CODE END RTOS_EVENTS */

/* Start scheduler */
osKernelStart();

/* We should never get here as control is now taken by the scheduler */

/* Infinite loop */
/* USER CODE BEGIN WHILE */
while (1) {
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

/** Configure the main internal regulator output voltage
 */
__HAL_RCC_PWR_CLK_ENABLE();
__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

/** Initializes the RCC Oscillators according to the specified parameters
 * in the RCC_OscInitTypeDef structure.
 */
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
RCC_OscInitStruct.HSIState = RCC_HSI_ON;
RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
RCC_OscInitStruct.PLL.PLLM = 16;
RCC_OscInitStruct.PLL.PLLN = 192;
RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
RCC_OscInitStruct.PLL.PLLQ = 4;
if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
	Error_Handler();
}

/** Initializes the CPU, AHB and APB buses clocks
 */
RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
	Error_Handler();
}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

/* USER CODE BEGIN ADC1_Init 0 */

/* USER CODE END ADC1_Init 0 */

ADC_ChannelConfTypeDef sConfig = { 0 };

/* USER CODE BEGIN ADC1_Init 1 */

/* USER CODE END ADC1_Init 1 */

/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
 */
hadc1.Instance = ADC1;
hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
hadc1.Init.Resolution = ADC_RESOLUTION_12B;
hadc1.Init.ScanConvMode = ENABLE;
hadc1.Init.ContinuousConvMode = ENABLE;
hadc1.Init.DiscontinuousConvMode = DISABLE;
hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
hadc1.Init.NbrOfConversion = 2;
hadc1.Init.DMAContinuousRequests = ENABLE;
hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
if (HAL_ADC_Init(&hadc1) != HAL_OK) {
	Error_Handler();
}

/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
 */
sConfig.Channel = ADC_CHANNEL_1;
sConfig.Rank = 2;
sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
	Error_Handler();
}

/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
 */
sConfig.Channel = ADC_CHANNEL_0;
sConfig.Rank = 1;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
	Error_Handler();
}
/* USER CODE BEGIN ADC1_Init 2 */

/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

/* USER CODE BEGIN SPI1_Init 0 */

/* USER CODE END SPI1_Init 0 */

/* USER CODE BEGIN SPI1_Init 1 */

/* USER CODE END SPI1_Init 1 */
/* SPI1 parameter configuration*/
hspi1.Instance = SPI1;
hspi1.Init.Mode = SPI_MODE_MASTER;
hspi1.Init.Direction = SPI_DIRECTION_2LINES;
hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
hspi1.Init.NSS = SPI_NSS_SOFT;
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
hspi1.Init.CRCPolynomial = 10;
if (HAL_SPI_Init(&hspi1) != HAL_OK) {
	Error_Handler();
}
/* USER CODE BEGIN SPI1_Init 2 */

/* USER CODE END SPI1_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

/* DMA controller clock enable */
__HAL_RCC_DMA2_CLK_ENABLE();

/* DMA interrupt init */
/* DMA2_Stream0_IRQn interrupt configuration */
HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
GPIO_InitTypeDef GPIO_InitStruct = { 0 };
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

/* GPIO Ports Clock Enable */
__HAL_RCC_GPIOC_CLK_ENABLE();
__HAL_RCC_GPIOH_CLK_ENABLE();
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOB_CLK_ENABLE();

/*Configure GPIO pin Output Level */
HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

/*Configure GPIO pin Output Level */
HAL_GPIO_WritePin(GPIOB, CS_Pin | RST_Pin | DC_Pin, GPIO_PIN_RESET);

/*Configure GPIO pin : LED_Pin */
GPIO_InitStruct.Pin = LED_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

/*Configure GPIO pin : PC15 */
GPIO_InitStruct.Pin = GPIO_PIN_15;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/*Configure GPIO pins : CS_Pin RST_Pin DC_Pin */
GPIO_InitStruct.Pin = CS_Pin | RST_Pin | DC_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

// configura PB0 como saída
GPIO_InitStruct.Pin = BUZZER_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

// garante que começa desligado
HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
/* USER CODE BEGIN 5 */
/* Infinite loop */
while (1) {
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

	osDelay(200);
}
/* USER CODE END 5 */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM4 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
/* USER CODE BEGIN Callback 0 */

/* USER CODE END Callback 0 */
if (htim->Instance == TIM4) {
	HAL_IncTick();
}
/* USER CODE BEGIN Callback 1 */

/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
/* USER CODE BEGIN Error_Handler_Debug */
/* User can add his own implementation to report the HAL error return state */
__disable_irq();
while (1) {
}
/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
