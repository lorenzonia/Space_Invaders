/*
 * game.h
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */

#ifndef INC_GAME_H_
#define INC_GAME_H_


#include "player.h"
#include "enemy.h"
#include "bullet.h"

typedef enum {
    MENU,
    PLAYING,
    GAME_OVER
} GameState;

typedef struct {
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Bullet bullets[MAX_BULLETS];

    int score;
    int level;
    GameState state;
} Game;

void Game_Init(Game *g);
void Game_Update(Game *g);
void Game_Reset(Game *g);

#endif /* INC_GAME_H_ */
