#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "world.h"

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

#endif // _ENGINE_H_
