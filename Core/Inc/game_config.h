#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

// ==========================
// TELA
// ==========================
#define SCREEN_WIDTH   160
#define SCREEN_HEIGHT  128

// ==========================
// HUD (parte superior fixa)
// ==========================
#define HUD_HEIGHT     12

// ==========================
// PLAYER
// ==========================
#define PLAYER_WIDTH   13
#define PLAYER_HEIGHT  10
#define PLAYER_SPEED   2

// HITBOX PLAYER
#define PLAYER_HITBOX_Y_OFFSET   5
#define PLAYER_HITBOX_HEIGHT    (PLAYER_HEIGHT + 5)

// ==========================
// ENEMY
// ==========================
#define ENEMY_WIDTH    8
#define ENEMY_HEIGHT   8

// ==========================
// MARGENS DE LIMPEZA
// ==========================
#define CLEAR_MARGIN_X   3
#define CLEAR_MARGIN_Y   3

#define BULLET_CLEAR_W   6
#define BULLET_CLEAR_H   10

#define ENEMY_CLEAR_SIZE 12
#define PLAYER_CLEAR_W   (PLAYER_WIDTH + 6)
#define PLAYER_CLEAR_H   (PLAYER_HEIGHT + 6)

// ==========================
// LIMITES DO JOGO
// ==========================
#define GAME_TOP       HUD_HEIGHT
#define GAME_BOTTOM    (SCREEN_HEIGHT - ENEMY_CLEAR_SIZE)


#endif
