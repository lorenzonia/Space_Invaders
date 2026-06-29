/*
 * player.c
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */

#include "player.h"

void Player_Init(Player *p)
{
    p->x = 60;
    p->y = 110;

    p->last_x = p->x;
    p->last_y = p->y;

    p->speed = 1;
}

void Player_Move(Player *p, int direcao)
{
    // salva posição anterior
    p->last_x = p->x;
    p->last_y = p->y;

    if (direcao == -1 && p->x > 0)
        p->x -= p->speed;

    if (direcao == 1 && p->x < (160 - PLAYER_WIDTH))
        p->x += p->speed;
}
