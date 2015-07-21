#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "world.h"
#include "object.h"

#define GM_EXIT 	(-1)
#define GM_TITLE	0
#define GM_GAME		1
#define GM_CUT		2
#define GM_GAMEOVER	3
#define GM_YOUWIN	4
#define GM_DEMO		5
#define GM_PAUSE	6
#define GM_SPLASH	7

#define SND_LASER_SHOOT		1
#define SND_SHORT_LASER_SHOOT	2
#define SND_ROCKET_SHOOT	3
#define SND_CANNON_SHOOT	4
#define SND_EXPLODE		5
#define SND_CONTACT		6
#define SND_MOVE		7
#define SND_ELEVATOR		8
#define SND_BONUS		9

#define MUSIC_STOP		0
#define MUSIC_INTRO		1
#define MUSIC_GAME		2
#define MUSIC_WIN		MUSIC_INTRO
#define MUSIC_LOSE		MUSIC_INTRO

#define MAX_GARAGES 16

typedef struct TGAMEDATA {
	int mode;
	int score;
	int base_level;
	int lives;
	int fuel;
	int health;
	int easy_mode;
	int hidden_level_entered;
	int garages[MAX_GARAGES][2];

	WORLD *world;
} TGAMEDATA;

#define MAX_LIGHTS 13

typedef struct {
	int x, y;
	float radius;
	float r, g, b;
} Light;

void GameLoop();

/* FIXME: this should not be public, remove later */
#define F_UP 0
#define F_RIGHT 1
#define F_DOWN 2
#define F_LEFT 3

unsigned char ChangeScreen(int flag);
void RestartLevel();
void InitNewScreen();


/* FIXME: Move to other header file (object.h ??) */
void BlowUpEnemy(TSHIP *gobj);
int IsTouch(int x, int y, TSHIP *gobj);
int IsOverlap(int x, int y, TSHIP *gobj1, TSHIP *gobj2);
void GetCurrentSpriteDimensions(TSHIP *i, int *cx, int *cy);
int FacingLeft(TSHIP *i);
int FacingRight(TSHIP *i);

extern TGAMEDATA *game;
extern unsigned int player_attached;
extern int elevator_flag;
extern int base_cur_screen;
extern int base_level_start;
extern unsigned char ship_cur_screen;
extern int game_level;

#endif // _ENGINE_H_
