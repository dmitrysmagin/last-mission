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
#define GM_EDITOR	8

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
	int ship_screen;
	int base_screen;
	int base_restart_screen;
	int lives;
	int fuel;
	int health;
	int easy_mode;
	int level;
	int screen_bonus;
	int player_attached;
	int elevator_flag;
	int hidden_level_entered;
	int ticks_for_damage;
	int garages[MAX_GARAGES][2];
	int mapx, mapy; /* current position in map */
	int rmapx, rmapy;

	WORLD *world;
} TGAMEDATA;

void SetGameMode(int mode);
void GameLoop();
void InitEnemies(int screen);
void BlitEnemies();

#define STATUS_YPOS 184

/* FIXME: this should not be public, remove later */
#define F_UP 0
#define F_RIGHT 1
#define F_DOWN 2
#define F_LEFT 3

int getscreen(int mapx, int mapy);
unsigned char ChangeScreen(int flag);
void RestartLevel();
void InitNewScreen();

int FacingLeft(TSHIP *i);
int FacingRight(TSHIP *i);

void Update_Ship(TSHIP *ship);
void Update_Base(TSHIP *base);

void UpdateScoreWithShip(TSHIP *gobj);
void DestroyHiddenAreaAccess(TSHIP *i, int playEffects);

extern TGAMEDATA *game;

#endif // _ENGINE_H_
