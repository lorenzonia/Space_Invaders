/*
 * bullet.h
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */

#ifndef INC_BULLET_H_
#define INC_BULLET_H_

#include <stdint.h>

#define MAX_BULLETS 5

typedef struct {
    int x;
    int y;
    int last_x;
    int last_y;
    int active;
} Bullet;

void Bullet_Fire(Bullet bullets[], int x, int y);
void Bullet_Update(Bullet bullets[]);

#endif /* INC_BULLET_H_ */
