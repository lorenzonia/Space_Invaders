/*
 * player.h
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */

#ifndef INC_PLAYER_H_
#define INC_PLAYER_H_

#include "game_config.h"

typedef struct {
    int x;
    int y;
    int last_x;
    int last_y;
    int speed;
} Player;

void Player_Init(Player *p);
void Player_Move(Player *p, int direcao);

#endif /* INC_PLAYER_H_ */
