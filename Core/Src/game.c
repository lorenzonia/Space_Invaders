/*
 * game.c
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */


#include "game.h"
#include "player.h"
#include "enemy.h"
#include "bullet.h"

void Game_Init(Game *g)
{
    Player_Init(&g->player);

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        g->enemies[i].active = 0;
        g->enemies[i].x = 0;
        g->enemies[i].y = 0;
        g->enemies[i].last_x = 0;
        g->enemies[i].last_y = 0;
        g->enemies[i].explosion_timer = 0;
    }

    // spawn inicial
    Enemy_Spawn(g->enemies, 3);

    g->score = 0;
    g->level = 1;
}

void Game_Reset(Game *g)
{
    Player_Init(&g->player);

    for (int i = 0; i < MAX_ENEMIES; i++)
        Enemy_Init(&g->enemies[i]);

    for (int i = 0; i < MAX_BULLETS; i++)
        g->bullets[i].active = 0;

    g->score = 0;
    g->level = 1;
}
