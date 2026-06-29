/*
 * enemy.h
 *
 *  Created on: 26 de mai. de 2026
 *      Author: marti
 */

#ifndef INC_ENEMY_H_
#define INC_ENEMY_H_


#define MAX_ENEMIES 10

typedef struct {
    float x;
    float y;
    float last_x;
    float last_y;
    int active;
    int explosion_timer;
} Enemy;

int contar_inimigos(Enemy enemies[]);

void Enemy_Init(Enemy *e);
void Enemy_Update(Enemy *e, float speed);
void Enemy_Spawn(Enemy enemies[], int quantidade);

#endif /* INC_ENEMY_H_ */
