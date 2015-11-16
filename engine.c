/*
	The Last Mission Remake

	Game engine, rewritten and reimagined :)
	Dmitry Smagin exmortis[REPLACE_WITH_AT_SYMBOL]yandex.ru

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_rotozoom.h>

#include "data.h"
#include "demo.h"
#include "random.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "object.h"
#include "object_enemy.h"
#include "object_garage.h"
#include "object_bfg.h"
#include "object_laser.h"
#include "engine.h"
#include "room.h"

#define GAME_START_SCREEN 1 // Start of the labyrinth.

int UpdateFuel();
void ReEnableBase(TSHIP *base);
void DoMachineGun(TSHIP *ship);
void DoRocketLauncher(TSHIP *ship);
void InitShip();
void InitEnemies();
void InitNewGame();
int UpdateLives();
void BlitStatus();
void BlitStatusData();
void DoTitle();
void DoWinScreen();
void DoGame();
void CreateWallExplosion(int x, int y);
void RenderGame(int renderStatus);

// Game callbacks.
void HitTheBonus(int);
void PublishScore();
void GameLevelUp();

int screen_procedure;
int frame_skip = 0;
int modern_background = 1;
int title_start_flag = 0;
int youwin_start_flag = 0;

/* Global pointer to internal game data, fix later*/
TGAMEDATA gamedata, *game = &gamedata;

void PlayMusicInt(int music)
{
	static int current_music = MUSIC_STOP;

	if (current_music != music) {
		current_music = music;
		PlayMusic(current_music);
	}
}

void SetGameMode(int mode)
{
	game->mode = mode;

	switch (mode) {
	case GM_TITLE:
		PlayMusicInt(MUSIC_INTRO);

		// To trigger enter actions.
		title_start_flag = 0;
		break;

	case GM_GAMEOVER:
		PlayMusicInt(MUSIC_LOSE);
		break;

	case GM_YOUWIN:
		PlayMusicInt(MUSIC_WIN);

		// To trigger enter actions.
		youwin_start_flag = 0;
		break;

	case GM_GAME:
	case GM_DEMO:
		PlayMusicInt(MUSIC_GAME);
		break;

	case GM_PAUSE:
		break;

	case GM_SPLASH:
		PlayMusicInt(MUSIC_STOP);
		break;
	}
}

unsigned char ChangeScreen(int flag)
{
	WORLD *world = game->world;
	int result;

	switch (flag) {
	case F_UP:
		result = (world->room + game->ship_screen)->up;
		break;
	case F_RIGHT:
		result = (world->room + game->ship_screen)->right;
		break;
	case F_DOWN:
		result = (world->room + game->ship_screen)->down;
		break;
	case F_LEFT:
		result = (world->room + game->ship_screen)->left;
		break;
	default:
		result = 0;
	}

	if (result == 0)
		return 0;

	TSHIP *ship = gObj_Ship();
	TSHIP *base = ship->base;

	// if base is blocking the way on the other screen
	if (result == game->base_screen) {
		if (flag == F_LEFT) {
			if (base->x >= SCREEN_WIDTH - 32 - 40 &&
			    ship->y + 12 >= base->y)
				return 0;
		}

		if (flag == F_RIGHT) {
			if(base->x < 32 && ship->y + 12 >= base->y)
				return 0;
		}
	}

	game->ship_screen = result;

	return 1;
}

inline int ShipBaseOffset(TSHIP *ship, TSHIP *base)
{
	int xs, xb;

	xs = gObj_GetWidth(ship);
	xb = gObj_GetWidth(base);

	return (xb - xs) / 2;
}

int FacingLeft(TSHIP *i)
{
	return i->cur_frame == i->max_frame;
}

int FacingRight(TSHIP *i)
{
	return i->cur_frame == i->min_frame;
}

int MaxRightPos(TSHIP *i)
{
	return SCREEN_WIDTH - gObj_GetWidth(i);
}

int MaxBottomPos(TSHIP *i)
{
	return ACTION_SCREEN_HEIGHT - gObj_GetHeight(i);
}

int UpdateFuel()
{
	static int fuel_cnt = 25;

	if (fuel_cnt == 0) {
		if (game->fuel == 0) {
			SetGameMode(GM_GAMEOVER);
			LM_ResetKeys();
			PutString(8*16, 8*10, "NO FUERZA");

			return 1;
		}

		game->fuel -= 1;
		fuel_cnt = 25;
	} else {
		fuel_cnt -= 1;
	}

	return 0;
}

void Update_Ship(TSHIP *ship)
{
	TSHIP *base = ship->base;
	static unsigned char fallflag = 1;
	static int dy;

	if (--game->ticks_for_damage < 0)
		game->ticks_for_damage = 0;

	switch (ship->i) {
	case SHIP_TYPE_ROCKET_LAUNCHER:
		DoRocketLauncher(ship);
		break;
	case SHIP_TYPE_MACHINE_GUN:
		DoMachineGun(ship);
		break;
	case SHIP_TYPE_BFG:
		DoBFG(ship);
		break;
	case SHIP_TYPE_LASER:
		DoLaser(ship);
		break;
	}

	// check if ships are attached
	if (base->state == SH_ACTIVE &&
	    base->ai_type != AI_EXPLOSION &&
	    game->player_attached == 0) {
		if (FacingLeft(ship) || FacingRight(ship)) {
			int xs, ys, xb;

			xs = gObj_GetWidth(ship);
			ys = gObj_GetHeight(ship);
			xb = gObj_GetWidth(base);

			if ((ship->x + xs / 2 == base->x + xb / 2) &&
			    (ship->y + ys == base->y)) {
				game->player_attached = 1;
				PlaySoundEffect(SND_CONTACT);
			}
		}
	}

	// exit if attach mode is ON
	if (game->player_attached == 1 ||
	    UpdateFuel() == 1 ||
	    ship->state == SH_DEAD) {
		return;
	}

	if (GKeys[KEY_RIGHT] == 1) {
		if (FacingRight(ship)) {
			if (gObj_CheckTouch(ship->x + 2, ship->y, ship) == 0) {
				ship->x += 2;
			} else {
				if (ship->x == MaxRightPos(ship) && ChangeScreen(F_RIGHT) == 1) {
					ship->x = 0;
					InitNewScreen();
					ReEnableBase(ship->base);
				}
			}
		} else {
			if (ship->anim_speed_cnt == 0) {
				ship->cur_frame -= 1;
				ship->anim_speed_cnt = ship->anim_speed;
			} else {
				ship->anim_speed_cnt -= 1;
			}
		}
	}

	if (GKeys[KEY_LEFT] == 1) {
		if (FacingLeft(ship)) {
			if (gObj_CheckTouch(ship->x - 2, ship->y, ship) == 0) {
				ship->x -= 2;
			} else {
				if (ship->x == 0 && ChangeScreen(F_LEFT) == 1) {
					ship->x = MaxRightPos(ship);
					InitNewScreen();
					ReEnableBase(ship->base);
				}
			}
		} else {
			if (ship->anim_speed_cnt == 0) {
				ship->cur_frame += 1;
				ship->anim_speed_cnt = ship->anim_speed;
			} else {
				ship->anim_speed_cnt -= 1;
			}
		}
	}

	if (GKeys[KEY_UP] == 1) {
		if ((ship->y & 1) != 0)
			dy = 1;
		else
			dy = 2;

		if (gObj_CheckTouch(ship->x, ship->y - dy, ship) == 0) {
			ship->y -= dy;
		} else {
			if (ship->y == 0 && ChangeScreen(F_UP) == 1) {
				ship->y = MaxBottomPos(ship);
				InitNewScreen();
				ReEnableBase(ship->base);
			}
		}
	} else {
		if (GKeys[KEY_DOWN] == 1) {
			if ((ship->y & 1) != 0)
				dy = 1;
			else
				dy = 2;

			if (gObj_CheckTouch(ship->x, ship->y + dy, ship) == 0) {
				ship->y += dy;
			} else {
				if (ship->y == MaxBottomPos(ship) && ChangeScreen(F_DOWN) == 1) {
					ship->y = 0;
					InitNewScreen();
					ReEnableBase(ship->base);
				}
			}
		} else {
			if (fallflag == 0) {
				if (gObj_CheckTouch(ship->x, ship->y + 1, ship) == 0)
					ship->y += 1;

				fallflag = 1;
				if (ship->y == MaxBottomPos(ship) && ChangeScreen(F_DOWN) == 1) {
					ship->y = 0;
					InitNewScreen();
					ReEnableBase(ship->base);
				}
			} else {
				fallflag -= 1;
			}
		}
	}

}

void ReEnableBase(TSHIP *base)
{
	if (game->base_screen != game->ship_screen) {
		base->state = SH_HIDDEN;
	} else {
		base->state = SH_ACTIVE;
	}
}

void Update_Base(TSHIP *base)
{
	TSHIP *ship = gObj_Ship();

	if (ship == NULL)
		return;

	// do smth if attach mode ON
	int playMoveSound = 0;

	if (game->player_attached == 1 && ship->ai_type != AI_EXPLOSION) {
		if (GKeys[KEY_RIGHT] == 1) {
			if (FacingRight(ship)) {
				base->cur_frame ^= 1;
				playMoveSound = 1;

				if (gObj_CheckTouch(ship->x + 2, ship->y, ship) + gObj_CheckTouch(base->x + 2, base->y, base) == 0) {
					if (GetTileI((base->x + 40) >> 3, (base->y + 16) >> 3)) {
						ship->x += 2;
						base->x += 2;
					}
				} else {
					if (base->x == 280 && ChangeScreen(F_RIGHT) == 1) {
						ship->x = ShipBaseOffset(ship, base);
						base->x = 0;
						game->base_screen = game->ship_screen;
						InitNewScreen();
					}
				}
			} else {
				if (ship->anim_speed_cnt == 0) {
					ship->cur_frame -= 1;
					ship->anim_speed_cnt = ship->anim_speed;
				} else {
					ship->anim_speed_cnt -= 1;
				}
			}
		}

		if (GKeys[KEY_LEFT] == 1) {
			if (FacingLeft(ship)) {
				base->cur_frame ^= 1;
				playMoveSound = 1;

				if (gObj_CheckTouch(ship->x - 2, ship->y, ship) + gObj_CheckTouch(base->x - 2, base->y, base) == 0) {
					if (GetTileI((base->x - 2) >> 3, (base->y + 16) >> 3)) {
						ship->x -= 2;
						base->x -= 2;
					}
				} else {
					if (base->x == 0 && ChangeScreen(F_LEFT) == 1) {
						//xxx
						ship->x = 280 + ShipBaseOffset(ship, base);
						base->x = 280;
						game->base_screen = game->ship_screen;
						InitNewScreen();
					}
				}
			} else {
				if (ship->anim_speed_cnt == 0) {
					ship->cur_frame += 1;
					ship->anim_speed_cnt = ship->anim_speed;
				} else {
					ship->anim_speed_cnt -= 1;
				}
			}
		}

		if (playMoveSound)
			PlaySoundEffect(SND_MOVE);
		else
			StopSoundEffect(SND_MOVE);

		if (GKeys[KEY_UP] == 1) {
			// if bound with base and standing on a bridge - don't allow to fly up
			for (int f = 0; f <= 3; f++) {
				if (GetTileI(((base->x + 8) >> 3) + f, (base->y + 16) >> 3) == 245)
					return;
			}

			// if standing on an elevator which is lifting up - don't allow to fly up
			if (game->elevator_flag == 1)
				return;

			StopSoundEffect(SND_MOVE);

			game->player_attached = 0;
			ship->y -= 2;
		}
	}
}

void AddScore(int update)
{
	const int points_per_life = 15000;

	int livesBefore = game->score / points_per_life;

	game->score += update;

	int livesAfter = game->score / points_per_life;

	game->lives += (livesAfter - livesBefore);
}

void UpdateScoreWithShip(TSHIP *gobj)
{
	int score;
	TSHIP *ship = gObj_Ship();

	if (ship == NULL)
		return;

	switch (gobj->ai_type) {
	case AI_SHIP:
	case AI_BASE:
	case AI_BULLET:
	case AI_SHOT:
	case AI_BFG_SHOT:
	case AI_HOMING_SHOT:
	case AI_BONUS:
	case AI_ELECTRIC_SPARKLE_VERTICAL:
	case AI_ELECTRIC_SPARKLE_HORIZONTAL:
		break;

	default:
		// /2 less points for hard mode, /2 less for rocket launcher of BFG
		score = gobj->ai_type * 100 * (game->level & 7) + (RandomInt() & 127);

		if (game->easy_mode)
			score >>= 1;

		if (ship->i == SHIP_TYPE_ROCKET_LAUNCHER || ship->i == SHIP_TYPE_BFG)
			score >>= 1;

		AddScore(score);
		break;
	}
}

void DestroyHiddenAreaAccess(TSHIP *i, int playEffects)
{
	// Cleanup background and make noise if needed.
	for (int y = 0; y < i->dy; ++y) {
		int ay = i->y + y;

		for (int x = 0; x < i->dx; ++x) {
			int ax = i->x + x;

			SetTileI(ax >> 3, ay >> 3, 0);

			if (playEffects && !(y % 16) && !(x % 16) && (ay + 16 < ACTION_SCREEN_HEIGHT)) {
				CreateWallExplosion(ax, ay);
			}
		}
	}

	if (playEffects) {
		PlaySoundEffect(SND_EXPLODE);
	}

	gObj_DestroyObject(i);
}

void DoMachineGun(TSHIP *ship)
{
	static int mg_timeout = 0;
	if (--mg_timeout < 0)
		mg_timeout = 0;

	if (GKeys[KEY_FIRE] == 1) {
		if (UpdateLaser(1) == 1) {
			gObj_Explode(ship);
			LM_ResetKeys();
			return;
		}

		if (!mg_timeout) {
			//if ship is not facing right or left, then exit.
			if (!FacingLeft(ship) && !FacingRight(ship))
				return;

			// Create a new bullet.
			TSHIP *bullet = gObj_CreateObject();
			bullet->i = 50;
			bullet->x = ship->x + (FacingRight(ship) ? 32 : -8);
			bullet->y = ship->y + 5;
			bullet->dy = 0;
			bullet->dx = FacingRight(ship) ? 5 : -5;
			bullet->anim_speed = 4;
			bullet->anim_speed_cnt = bullet->anim_speed;
			bullet->move_speed_cnt = bullet->move_speed;
			bullet->cur_frame = FacingRight(ship) ? 0 : 1;
			gObj_Constructor(bullet, AI_SHOT);
			bullet->parent = ship;

			// Reset timeout.
			mg_timeout = 20;

			PlaySoundEffect(SND_SHORT_LASER_SHOOT);
		}
	} else {
		UpdateLaser(-1);
		mg_timeout = 0;
	}
}

void DoRocketLauncher(TSHIP *ship)
{
	static int rl_timeout = 0;
	if (--rl_timeout < 0)
		rl_timeout = 0;

	if (GKeys[KEY_FIRE] == 1) {
		if (!rl_timeout) {
			//if ship is not facing right or left, then exit.
			if (!FacingLeft(ship) && !FacingRight(ship))
				return;

			// Create a new bullet.
			TSHIP *bullet = gObj_CreateObject();
			bullet->i = 54;
			bullet->y = ship->y + 1;
			bullet->dy = 0;
			bullet->anim_speed = 4;
			bullet->anim_speed_cnt = bullet->anim_speed;
			bullet->move_speed_cnt = bullet->move_speed;
			gObj_Constructor(bullet, AI_HOMING_SHOT);
			bullet->parent = ship;

			if (FacingRight(ship)) {
				// to the right
				bullet->x = ship->x + 32;
				bullet->dx = 3 ;
				bullet->cur_frame = 2;
				bullet->min_frame = 2;
				bullet->max_frame = 3;
			} else {
				// to the left
				bullet->x = ship->x - 14;
				bullet->dx = -3;
				bullet->cur_frame = 0;
				bullet->max_frame = 1;
			}

			PlaySoundEffect(SND_ROCKET_SHOOT);

			// Reset timeout.
			rl_timeout = 30;
		}
	} else {
		//rl_timeout --;
	}
}

void CreateWallExplosion(int x, int y)
{
	TSHIP *j = gObj_CreateObject();
	j->i = 7;
	j->x = x;
	j->y = y;
	j->anim_speed = 6;
	j->anim_speed_cnt = 6;
	j->max_frame = 2;
	gObj_Constructor(j, AI_EXPLOSION);
}

void InitShip()
{
	TSHIP *base, *ship;

	GarageRestore();

	/* Erase all game objects and init object queue */
	gObj_DestroyAll();

	/* FIXME: strictly ship is 1st, base is second */
	ship = gObj_CreateObject();
	base = gObj_CreateObject();

	// base data
	base->x = 148;
	base->y = 104;
	base->i = 1;
	base->min_frame = 0;
	base->cur_frame = 0;
	base->max_frame = 1;
	base->anim_speed = 0;
	base->anim_speed_cnt = 0;
	base->parent = ship;
	gObj_Constructor(base, AI_BASE);

	// flying ship data
	ship->i = GetPlayerShipIndex();
	ship->x = 148 + ShipBaseOffset(ship, base);
	ship->y = 68;
	ship->base = base;
	ship->smoke = NULL;
	ship->laser = NULL;

	switch (ship->i) {
	case SHIP_TYPE_LASER:
	case SHIP_TYPE_MACHINE_GUN:
	case SHIP_TYPE_ROCKET_LAUNCHER:
		ship->max_frame = 6;
		ship->min_frame = 0;
		break;

	case SHIP_TYPE_OBSERVER:
		ship->max_frame = 3;
		ship->min_frame = 1;
		break;

	case SHIP_TYPE_BFG:
		ship->min_frame = 0;
		ship->max_frame = 4;
		break;
	}

	ship->cur_frame = ((game->level & 1) == 0 ? ship->max_frame : ship->min_frame);
	ship->anim_speed = 1;
	ship->anim_speed_cnt = 1;
	gObj_Constructor(ship, AI_SHIP);
}

void InitEnemies()
{
	ROOM *room = game->world->room + game->ship_screen;
	OBJECT *object = room->object;
	int count = room->object_num;

	/* Clear all except ship/base and smoke */
	TSHIP *gobj = gObj_First();
	for (; gobj; gobj = gObj_Next(gobj)) {
		switch (gobj->ai_type) {
		case AI_SMOKE:
			/* reset smoke animation, otherwise it appears at wrong
			   place when changing screen */
			gobj->anim_speed_cnt = 0;
			gobj->cur_frame = gobj->max_frame;
			break;
		case AI_SHIP:
			gobj->laser = NULL;
		case AI_BASE:
			break;
		default:
			gObj_DestroyObject(gobj);
			break;
		}
	}

	screen_procedure = room->procedure;
	game->screen_bonus = room->bonus;

	for (; count > 0; count--, object++) {

		if (object->index == BONUS_FACEBOOK || object->index == BONUS_TWITTER) {
			if (game->mode == GM_DEMO || game->base_screen < game->ship_screen)
				continue;
		}

		TSHIP *en = gObj_CreateObject();
		en->i = object->index;
		en->x = object->x;
		en->y = object->y;
		en->anim_speed = object->speed;
		en->anim_speed_cnt = object->speed;
		en->min_frame = object->minframe;
		en->cur_frame = object->minframe;
		en->max_frame = object->maxframe;
		gObj_Constructor(en, object->ai);
		en->move_speed = 1; // standard
		en->move_speed_cnt = 1;

		/* FIXME: Special hack */
		if (en->i == 11 || en->i == 42)
			en->flags &= ~GOBJ_DESTROY;

		/* FIXME: Hack for homing missiles:
				make them undestroyable in hard mode */
		if (en->ai_type == AI_HOMING_MISSLE && !game->easy_mode) {
			en->flags &= ~(GOBJ_SOLID|GOBJ_DESTROY);
		}

		if (en->ai_type == AI_GARAGE) {
			CreateGarage(en, object->garage_id);
		} else if (en->ai_type == AI_HIDDEN_AREA_ACCESS) {
			en->dx = object->speed;
			en->dy = object->minframe;

			if (game->hidden_level_entered) {
				DestroyHiddenAreaAccess(en, 0);
			}
		}
	}
}

void InitNewScreen()
{
	UnpackLevel(game->world, game->ship_screen);

	InitEnemies();

	if (game->ship_screen == 92) {
		// Memorize that we entered the hidden area once.
		game->hidden_level_entered = 1;
	} else if (game->hidden_level_entered && game->ship_screen == 1) {
		// Save game, cause we are back from the underggroung (most probably, heh).
		GarageSave();
		PublishScore();

	}

	CleanupBfg();
}

int GameLevelFromScreen(int screen)
{
	int levels[10] = {
		8, 15, 22, 29, 36, 43, 50, 55, 62, 70
	};

	for (int i = 0; i < 10; ++i) {
		if (levels[i] > screen)
			return i + 1;
	}

	return 10;
}

void InitNewGame()
{
	game->fuel = 5000;
	game->lives = 10;
	game->health = 3;
	game->score = 0;

	game->ship_screen = GAME_START_SCREEN;
	game->base_screen = GAME_START_SCREEN;
	game->base_restart_screen = GAME_START_SCREEN;
	game->level = GameLevelFromScreen(GAME_START_SCREEN);

	game->player_attached = 0;
	game->hidden_level_entered = 0;

	InitGaragesForNewGame();
	InitShip();
	InitNewScreen();

	BlitStatus();
}

int UpdateLives()
{
	game->lives -= 1;
	game->health = 3;

	// Just in case they are playing.
	StopSoundEffect(SND_ELEVATOR);
	StopSoundEffect(SND_MOVE);
	PublishScore();

	if (game->lives == 0) {
		SetGameMode(GM_GAMEOVER);
		LM_ResetKeys();
		PutString(8*16, 8*10, "HAS PERDIDO");

		return 1;
	}

	return 0;
}

void RestartLevel()
{
	if (UpdateLives() == 1)
		return;

	game->player_attached = 0;
	game->ship_screen = game->base_restart_screen;
	game->base_screen = game->base_restart_screen;
	game->elevator_flag = 0;

	InitShip();
	InitNewScreen();
}

void BlitStatus()
{
	for (int i = 0; i < 7; i++) {
		PutStream(0, STATUS_YPOS + i * 8, &STATUSBAR1[i * 43 + 2]);
	}
}

void BlitStatusData()
{
	static char string_buffer[16];

	// level
	sprintf(string_buffer, "%02d", game->level);
	PutString(8*16, STATUS_YPOS + 16, string_buffer);

	// fuel
	sprintf(string_buffer, "%04d", game->fuel);
	PutString(8*14, STATUS_YPOS + 24, string_buffer);

	// score
	sprintf(string_buffer, "%08d", game->score);
	PutString(8*10, STATUS_YPOS + 32, string_buffer);

	// health bar.
	string_buffer[2] = 0;
	for (int y = 0; y < 3; ++y) {
		string_buffer[1] = string_buffer[0] = (game->health > y) ? 84 : 88;
		PutStream(8*19, STATUS_YPOS + 8 * (4 - y), (unsigned char *)string_buffer);
	}

	// laser
	BlitLaserStatus();

	// lives
	sprintf(string_buffer, "%02d", game->lives);
	PutString(8*28, STATUS_YPOS + 24, string_buffer);

	// score record
	PutString(8*22, STATUS_YPOS + 32, "88888888");

}

SDL_Surface *logo;
float factor, delta;

void LoadLogo()
{
	logo = SDL_LoadBMP("graphics/logo.bmp");

	factor = 1.0;
	delta = -1.0 / (float)(logo->h / 2);
}

void RotateLogo()
{
	static int speed = 2;
	extern SDL_Surface *small_screen;
	SDL_Surface *scaled_logo;
	SDL_Rect dst;

	if (speed > 0) {
		speed -= 1;
		return;
	}

	speed = 2;

	factor += delta;

	if (factor >= 1.0) {
		factor = 1.0;
		delta = -delta;
	}

	if (factor <= -1.0) {
		factor = -1.0;
		delta = -delta;
	}

	dst.x = 96;
	dst.y = 142 + GAME_YPOS;
	dst.w = logo->w;
	dst.h = logo->h;

	SDL_FillRect(small_screen, &dst, 0);

	dst.y += logo->h / 2 - fabs(factor) * logo->h / 2;

	scaled_logo = zoomSurface(logo, 1.0, factor, 0);
	SDL_BlitSurface(scaled_logo, 0, small_screen, &dst);
	SDL_FreeSurface(scaled_logo);
}

void LoadSplash()
{
	extern SDL_Surface *small_screen;
	char name[256];
	SDL_Rect dst;

	sprintf(name, "graphics/title%i.bmp", (SDL_GetTicks() & 3) + 1);
	SDL_Surface *title = SDL_LoadBMP(name);

	dst.x = (small_screen->w - title->w) / 2;
	dst.y = (small_screen->h - title->h) / 2;

	SDL_BlitSurface(title, NULL, small_screen, &dst);
	SDL_FreeSurface(title);
}

void DoSplash()
{
	static int ticks_for_splash = 0;

	if (ticks_for_splash == 0) {
		LoadSplash();
	}

	ticks_for_splash += 1;

	if (ticks_for_splash > 350)
		SetGameMode(GM_TITLE);

	if (LM_AnyKey() == 1) {
		LM_ResetKeys();
		ClearScreen();
		SetGameMode(GM_TITLE);
	}
}

int ticks_before_demo = 0;

void ResetDemoTicksCounter()
{
	ticks_before_demo = 0;
}

void DoTitle()
{
	if (title_start_flag == 0) {
		FillScreen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
		game->ship_screen = 0;
		title_start_flag = 1;
		InitNewScreen();
		BlitLevel(game->ship_screen);
		PutSpriteI(50*4, 108, 45, 0);
		PutString(76, 88 + GAME_YPOS, "ESPACIO PARA COMENZAR");
		PutString(60, 24 + GAME_YPOS, "ORIGINAL GAME: PEDRO RUIZ");
		PutString(76, 36 + GAME_YPOS, "REMAKE: DMITRY SMAGIN");
		PutString(140, 44 + GAME_YPOS, "ALEXEY PAVLOV");
		PutString(60, 56 + GAME_YPOS, "MUSIC AND SFX: MARK BRAGA");
		ResetDemoTicksCounter();
	}

	RotateLogo();

	ticks_before_demo++;

	// wait some time before switching to demo mode
	if (ticks_before_demo >= 3660) { // wait 1 min
		game->easy_mode = 0;
		ResetDemo();
		SetGameMode(GM_DEMO);
		LM_ResetKeys();
		InitNewGame();
		return;
	}

	// exit to os
	if (Keys[SC_ESCAPE] == 1) {
		SetGameMode(GM_EXIT);
		return;
	}

	// start
	if (Keys[SC_SPACE] == 1 || Keys[SC_ENTER] == 1) {
		game->easy_mode = 1;
		ClearScreen();
		SetGameMode(GM_GAME);
		LM_ResetKeys();
		InitNewGame();
	}
}

void DoWinScreen()
{
	static int x_string = 0;
	static int win_ticks = 0;
	static char win_string[410] = ""
	"                                        "
	"ATENCION    ATENCION     TRANSMISION A LA NAVE EXPLORER           "
	"HAS CUMPLIDO TU ULTIMA MISION Y DEBES RETORNAR AL PLANETA NOVA DE LA GALAXIA TRAION"
	"                                        "
	"TU LUCHA NO HA SIDO EN VANO PUES LA LEJANA COLONIA DEL IMPERIO LLAMADA TIERRA HA SIDO "
	"LIBERADA DE LOS INVASORES Y PUEDE SER HABITADA DE NUEVO               REPITO  MENSAJE";

	if (youwin_start_flag == 0) {
		FillScreen(0, 144, SCREEN_WIDTH, 56, 0);
		youwin_start_flag = 1;
		x_string = 0;
		win_ticks = 0;
	} else {
		win_ticks++;
	}

	PutString(0 - x_string % 8, 20*8, &win_string[0] + x_string/8);

	if (x_string/8 >= sizeof(win_string))
		x_string = 0;
	else
		x_string += 1;

	if (LM_AnyKey() == 1 && win_ticks > 300) { // Ignore input first 5 seconds.
		SetGameMode(GM_TITLE);
		LM_ResetKeys();
	} else {
		TSHIP *ship = gObj_Ship();

		if (ship == NULL)
			return;

		// Update animations and screen.
		GKeys[KEY_RIGHT] = (ship->x < 93) ? 1 : 0;
		GKeys[KEY_LEFT] = 0;
		GKeys[KEY_UP] = (ship->x < 93 && ship->y > 40 ) ? 1 : 0;
		GKeys[KEY_DOWN] = 0;
		GKeys[KEY_FIRE] = 0;

		Update_Ship(ship);

		TSHIP *gobj = gObj_First();
		for (; gobj; gobj = gObj_Next(gobj))
			UpdateAnimation(gobj);

		if (!frame_skip)
			RenderGame(0);
	}
}

void DoKeys()
{
	// if not demo mode
	if (game->mode != GM_DEMO) {
		GKeys[KEY_LEFT] = Keys[SC_LEFT];
		GKeys[KEY_RIGHT] = Keys[SC_RIGHT];
		GKeys[KEY_UP] = Keys[SC_UP];
		GKeys[KEY_DOWN] = Keys[SC_DOWN];
		GKeys[KEY_FIRE] = Keys[SC_SPACE];
		GKeys[KEY_PAUSE] = Keys[SC_ENTER];
		GKeys[KEY_QUIT] = Keys[SC_ESCAPE];
	}
}

void BlitEnemies()
{
	for (TSHIP *gobj = gObj_First(); gobj; gobj = gObj_Next(gobj)) {
		if (gobj->ai_type == AI_GARAGE) {
#ifdef _DEBUG
			DrawRect(
				gobj->x, gobj->y,
				GARAGE_WIDTH, GARAGE_HEIGHT,
				gobj->garage_inactive ? 10 : 28);
#endif
		} else if (gobj->ai_type == AI_HIDDEN_AREA_ACCESS) {
#ifdef _DEBUG
			DrawRect(gobj->x, gobj->y, gobj->dx, gobj->dy, 10);
#endif
		}

		if (gobj->ai_type == AI_LASER)
			BlitLaser(gobj);
		else
		if (gobj->flags & GOBJ_VISIBLE)
			PutSpriteI(gobj->x, gobj->y, gobj->i, gobj->cur_frame);
	}
}

void BlitEnemyOutlines(WORLD *world)
{
	unsigned int shadow = (world->room + game->ship_screen)->shadow;

	for (TSHIP *gobj = gObj_First(); gobj; gobj = gObj_Next(gobj)) {
		/* Blit shadow only if object needs it */
		if (gobj->flags & GOBJ_SHADOW)
			PutSpriteS(gobj->x, gobj->y, gobj->i, gobj->cur_frame, shadow);
	}
}

void RenderGame(int renderStatus)
{
	if (modern_background) {
		BlitBackground(game->world, game->ship_screen); // blit background
		BlitLevelOutlines(game->world, game->ship_screen);
		BlitEnemyOutlines(game->world); // draw moving objects' outlines (shadows)
	} else {
		EraseBackground(0);
	}

	BlitLevel(game->ship_screen); // blit walls
	BlitBfg();
	BlitEnemies(); // draw all enemies and cannon+base

	if (renderStatus)
		BlitStatusData(); // draw score etc
}

void DoGame()
{
	switch(game->mode) {
	case GM_TITLE:
		// do title here
		DoTitle();
		break;

	case GM_DEMO:
		// demo mode here

		// if end playing demo
		if (PlayDemo() == 1 || LM_AnyKey() == 1) {
			SetGameMode(GM_TITLE);
			LM_ResetKeys();
			game->score = 0;
			break;
		}

	case GM_GAME:
		DoKeys();

		if (GKeys[KEY_PAUSE] == 1) {
			PutString(8*17, 8*17, "PAUSA");
			SetGameMode(GM_PAUSE);
			Keys[SC_ENTER] = 0;
			break;
		}

#ifdef _DEBUG
		{
			static char screen_nr_text[16];
			sprintf(screen_nr_text, "SCREEN %i", game->ship_screen);
			PutString(8*17, 8*17, screen_nr_text);
		}
#endif
		RecordDemo();

		if (Keys[SC_ESCAPE] == 1) {
			ClearScreen();
			SetGameMode(GM_TITLE);
			LM_ResetKeys();
			break;
		}

		// win the game:
		if (screen_procedure == 3 /*&& game->base_screen >= 70*/) {
			SetGameMode(GM_YOUWIN);

			LM_ResetKeys();
			PublishScore();
			return;
		}

		// do enemies
		for (TSHIP *gobj = gObj_First(); gobj; gobj = gObj_Next(gobj))
			gObj_Update(gobj);

		if (!frame_skip)
			RenderGame(1);
		break;

	case GM_PAUSE:
			DoKeys();

			if (GKeys[KEY_PAUSE] == 1) {
				PutString(8*17, 8*17, "     ");
				SetGameMode(GM_GAME);
				Keys[SC_ENTER] = 0;
			}
		break;

	case GM_GAMEOVER:
		if (LM_AnyKey() == 1) {
			InitNewGame();
			LM_ResetKeys();
			SetGameMode(GM_TITLE);
		}
		break;

	case GM_YOUWIN:
		DoWinScreen();
		break;

	case GM_SPLASH:
		DoSplash();
		break;
	}

}

int GameMode()
{
	return game->mode;
}

void SetEasyLevel(int level)
{
	game->easy_mode = level;
}

void HitTheBonus(int param)
{
}

void PublishScore()
{
}

void GameLevelUp()
{
}

void GameLoop()
{
	static char infostring[16] = "FPS: ";
	static int next_game_tick = 0;
	static int sleep_time = 0, frames = 0, frame_end = 0, frame_start = 0;
	static int show_fps = 0, max_frameskip = 0;

	SetGameMode(GM_SPLASH);
	game->world = load_world("data/lastmission.dat");

	LoadLogo();
	LoadSprites();

	next_game_tick = SDL_GetTicks();

	// main loop
	while (1) {
		if (frames == 0)
			frame_start = next_game_tick;

		frame_skip = 0;
		LM_PollEvents();
		DoGame();

		if (show_fps == 1)
			PutString(8*0, 8*17, &infostring[0]);

		LM_GFX_Flip();

		next_game_tick += 17; // gcc rounds (1000 / 60) to 16, but we need 17

		frames += 1;
		frame_end = SDL_GetTicks();

		if (frame_end - frame_start >= 1000) {
			if (show_fps == 1)
				sprintf(infostring, "FPS: %d", frames);
			frames = 0;
		}

		sleep_time = next_game_tick - frame_end;

		if(sleep_time > 0) {
			SDL_Delay(sleep_time);
		} else { // slow computer, do frameskip
			while (sleep_time < 0 && frame_skip < max_frameskip) { // max frame skip
				sleep_time += 17; // 1000/60
				frame_skip += 1;
				SDL_Delay(2);
				LM_PollEvents();
				DoGame();
			}
			next_game_tick = SDL_GetTicks();
		}

		if (Keys[SC_BACKSPACE] == 1) {
			max_frameskip ^= 1;
			Keys[SC_BACKSPACE] = 0;
		}

		if (game->mode == GM_EXIT)
			break;
	}

	free_world(game->world);
}

void LoadGame(TGAMEDATA *data)
{
	game->player_attached = 0;
	ResetLaser();
	SetGameMode(GM_GAME);
	game->elevator_flag = 0;

	game->easy_mode = data->easy_mode;

	game->hidden_level_entered = data->hidden_level_entered;
	game->fuel = data->fuel;
	game->lives = data->lives;
	game->health = data->health;
	game->score = data->score;

	game->ship_screen = data->base_screen;
	game->base_screen = data->base_screen;
	game->base_restart_screen = data->base_screen;
	game->level = GameLevelFromScreen(data->base_screen);

#if 0
	/* FIXME: Later */
	memcpy(garage_data, data->garages, sizeof(garage_data));
	memcpy(main_garage_data, data->garages, sizeof(main_garage_data));
#endif

	InitShip();
	InitNewScreen();

	for (int f = 0; f <= 6; f++) {
		PutStream(0, STATUSBAR1[f * 43 + 1] * 8, &STATUSBAR1[f * 43 + 2]);
	}
}

void SaveGame(TGAMEDATA *data)
{
	data->score = game->score;
	data->base_screen = game->base_restart_screen;
	data->lives = game->lives;
	data->hidden_level_entered = game->hidden_level_entered;
	data->fuel = game->fuel;
	data->health = game->health;
	data->easy_mode = game->easy_mode;
#if 0
	/* FIXME: Later */
	memcpy(data->garages, main_garage_data, sizeof(main_garage_data));
#endif
}

void ResetGame(int gameMode)
{
	game->player_attached = 0;
	game->base_restart_screen = 1;
	game->level = 1;
	game->fuel = 5000;
	game->lives = 10;
	game->hidden_level_entered = 0;
	game->health = 3;
	game->score = 0;
	ResetLaser();
	SetGameMode(gameMode);
	game->elevator_flag = 0;

	InitNewGame();
}
