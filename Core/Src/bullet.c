/*
 * bullet.c
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */

#include "bullet.h"

void Bullet_Fire(Bullet bullets[], int x, int y)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].last_y = y;
            bullets[i].active = 1;
            break;
        }
    }
}

void Bullet_Update(Bullet bullets[])
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            bullets[i].last_y = bullets[i].y;
            bullets[i].y -= 6;

            if (bullets[i].y < 0)
                bullets[i].active = 0;
        }
    }
}
