/*
 * enemy.c
 *
 * Created on: 26 de mai. de 2026
 * Author: marti
 */

#include "cmsis_os.h"
#include "enemy.h"
#include "game_config.h"
#include <stdlib.h>

void Enemy_Init(Enemy *e)
{
    // O C converte o resultado inteiro de rand() automaticamente para float
    e->x = (float)(rand() % (SCREEN_WIDTH - ENEMY_WIDTH));
    e->y = (float)(GAME_TOP + (rand() % 20));

    e->last_x = e->x;
    e->last_y = e->y;

    e->active = 1;
    e->explosion_timer = 0;
}

void Enemy_Update(Enemy *e, float speed) // <-- Alterado de int para float aqui
{
    if (e->active)
    {
        e->last_y = e->y;   // salva posição antiga
        e->y += speed;      // soma o float perfeitamente
    }
}

void Enemy_Spawn(Enemy enemies[], int quantidade)
{
    int spawned = 0;

    for (int i = 0; i < MAX_ENEMIES && spawned < quantidade; i++ )
    {
        if (!enemies[i].active)
        {
            int x_valido = 0;
            float novo_x = 0;
            int tentativas = 0;

            // Loop para garantir que o X sorteado não colida com ninguém já ativo
            while (!x_valido && tentativas < 15)
            {
            	unsigned int semente_extra = rand() + (unsigned int)osKernelGetTickCount();
            	novo_x = (float)(semente_extra % (SCREEN_WIDTH - ENEMY_WIDTH));
                x_valido = 1; // Assume que é bom, até testar com os vizinhos

                // Varre os outros inimigos para checar proximidade horizontal
                for (int j = 0; j < MAX_ENEMIES; j++)
                {
                    if (enemies[j].active)
                    {
                        // Se o X sorteado for muito perto de outro inimigo ativo no topo, rejeita.
                        // Damos uma margem segura: a largura do inimigo + 6 pixels de folga lateral.
                        if (abs((int)novo_x - (int)enemies[j].x) < (ENEMY_WIDTH + 6))
                        {
                            // Mas só nos importamos se o inimigo antigo ainda estiver perto do topo (Y inicial)
                            if (enemies[j].y < (GAME_TOP + 30))
                            {
                                x_valido = 0; // Rejeita e força o "while" a sortear outro X
                                break;
                            }
                        }
                    }
                }
                tentativas++;
            }

            // Ativa e posiciona o inimigo na coordenada X validada
            enemies[i].active = 1;
            enemies[i].x = novo_x;

            // CORREÇÃO CRÍTICA: Todos nascem RIGOROSAMENTE na mesma linha Y de partida.
            // Eliminar o 'rand() % 20' remove o desalinhamento que confundia as borrachas pretas.
            enemies[i].y = (float)GAME_TOP;

            // ESSENCIAL: Garante que o rastro comece limpo na mesma posição float de nascimento
            enemies[i].last_x = enemies[i].x;
            enemies[i].last_y = enemies[i].y;

            enemies[i].explosion_timer = 0;

            spawned++;
        }
    }
}

int contar_inimigos(Enemy enemies[])
{
    int cont = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            cont++;
        }
    }
    return cont;
}
