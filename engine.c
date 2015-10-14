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

#include "m_data.h"
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
void ReEnableBase();
void DoMachineGun();
void DoRocketLauncher();
void InitShip();
void InitEnemies();
void InitNewGame();
int UpdateLives();
void BlitStatus();
void DoTitle();
void DoWinScreen();
void DoGame();
void CreateWallExplosion(int x, int y);
void RenderGame(int renderStatus);

// Game callbacks.
void HitTheBonus(int);
void PublishScore();
void GameLevelUp();

unsigned char ship_cur_screen = 0;

unsigned int player_attached = 0;
int base_cur_screen;
int base_level_start = 1;
char screen_procedure;
int screen_bridge = 0;
int game_level = 1;
int ticks_for_damage = 0;
int sn_enabled = 1; // Facebook/Twitter integration.
int elevator_flag = 0; // 1 if elevator is working
int frame_skip = 0;
int modern_background = 1;
int title_start_flag = 0;
int youwin_start_flag = 0;
int cur_screen_bonus = 0;
int hidden_level_entered = 0;

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
		result = (world->room + ship_cur_screen)->up;
		break;
	case F_RIGHT:
		result = (world->room + ship_cur_screen)->right;
		break;
	case F_DOWN:
		result = (world->room + ship_cur_screen)->down;
		break;
	case F_LEFT:
		result = (world->room + ship_cur_screen)->left;
		break;
	default:
		result = 0;
	}

	if (result == 0)
		return 0;

	TSHIP *base = gObj_Base();
	TSHIP *ship = gObj_Ship();

	// if base is blocking the way on the other screen
	if (result == base_cur_screen) {
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

	if (player_attached == 1)
		screen_bridge = 1;
	else
		screen_bridge = 0;

	ship_cur_screen = result;

	return 1;
}

int ShipBaseOffset()
{
	int xs, xb;

	xb = gObj_GetWidth(gObj_Base());
	xs = gObj_GetWidth(gObj_Ship());

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

int gObj_CheckOverlap(int x, int y, TSHIP *gobj1, TSHIP *gobj2)
{
	int xs, ys, xs2, ys2;

	ys = y + gObj_GetHeight(gobj1);
	xs = x + gObj_GetWidth(gobj1);

	ys2 = gobj2->y + gObj_GetHeight(gobj2);
	xs2 = gobj2->x + gObj_GetWidth(gobj2);

	// Return 1 if rectangles do intersect.
	if (x < gobj2->x)
		if (xs > gobj2->x)
			goto _wdw2;

	if (x >= gobj2->x) {
		if (xs2 > x) {
		_wdw2:
			if ((y < gobj2->y && ys > gobj2->y) ||
			    (y >= gobj2->y && ys2 > y))
				return 1;
		}
	}

	return 0;
}

/* Check if gobj1 can destroy gobj2 */
int gObj_CheckDestruction(TSHIP *gobj1, TSHIP *gobj2)
{
	// 0 - any obj, 1 - player, 2 - weapon
	int obj1_type = (gobj1->flags & (GOBJ_PLAYER|GOBJ_WEAPON)) >> 5;
	int obj2_type = (gobj2->flags & (GOBJ_PLAYER|GOBJ_WEAPON)) >> 5;

	/* don't interact with the same objects */
	if (obj1_type == obj2_type)
		goto _exit_proc;

	/* FIXME: hack for garage, later make more generic */
	if(gobj2->ai_type == AI_GARAGE)
		goto _exit_proc;

	if (gobj1->ai_type == AI_BONUS) {
		/* Bonus hit by a bullet. Swap bonus. */
		if (obj2_type == 2) {
			gobj1->explosion.regenerate_bonus = 1;
		}

		gObj_Explode(gobj1);
		goto _exit_proc;
	} else if (gobj2->ai_type == AI_BONUS) {
		/* Bonus hit by a bullet. Swap bonus. */
		if (obj1_type == 2) {
			gobj2->explosion.regenerate_bonus = 1;
		}

		gObj_Explode(gobj2);
		goto _exit_proc;
	}

	if (gobj1->flags & GOBJ_HURTS || gobj2->flags & GOBJ_HURTS) {
			if (gobj1->ai_type != AI_BFG_SHOT)
				gObj_Explode(gobj1);

			if (gobj2->ai_type != AI_BFG_SHOT)
				gObj_Explode(gobj2);
	}

_exit_proc:

	/* FIXME: hack for laser */
	if (gobj1->ai_type == AI_LASER || gobj2->ai_type == AI_LASER)
		return 1;

	if (!(gobj1->flags & GOBJ_SOLID) || !(gobj2->flags & GOBJ_SOLID))
		return 0;

	return 1;
}

int IsTouch(int x, int y, TSHIP *gobj)
{
	int xs, ys;

	if (x < 0 || y < 0)
		return 1;

	ys = y + gObj_GetHeight(gobj);
	xs = x + gObj_GetWidth(gobj);

	if (xs > SCREEN_WIDTH || ys > ACTION_SCREEN_HEIGHT)
		return 1;

	TSHIP *en = gObj_First(0);

	for (; en; en = gObj_Next(en)) {
		/* don't compare with itself */
		if (en == gobj)
			continue;

		/* Ignore your weapons */
		if (en == gobj->parent || en->parent == gobj)
			continue;

		if (gObj_CheckOverlap(x, y, gobj, en)) {
			if (!gObj_CheckDestruction(gobj, en))
				continue;

			return 1;
		}
	}

	for (int dy = y; dy < ys; dy++) {
		for (int dx = x; dx < xs; dx++) {
			unsigned char b = GetTileI(dx >> 3, dy >> 3);

			if (b == 0)
				continue;

			// All tiles are solid.
			if (gobj->ai_type == AI_SHIP && b >= 246) {
				// ship dies from touching certain tiles
				gObj_Explode(gobj);
			}

			return 1;
		}
	}

	return 0;
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
	TSHIP *base = gObj_Base();
	static unsigned char fallflag = 1;
	static int dy;

	if (--ticks_for_damage < 0)
		ticks_for_damage = 0;

	switch (ship->i) {
	case SHIP_TYPE_ROCKET_LAUNCHER:
		DoRocketLauncher();
		break;
	case SHIP_TYPE_MACHINE_GUN:
		DoMachineGun();
		break;
	case SHIP_TYPE_BFG:
		DoBFG();
		break;
	case SHIP_TYPE_LASER:
		DoLaser();
		break;
	}

	// check if ships are attached
	if (base->state == SH_ACTIVE &&
	    base->ai_type != AI_EXPLOSION &&
	    player_attached == 0) {
		if (FacingLeft(ship) || FacingRight(ship)) {
			int xs, ys, xb;

			xs = gObj_GetWidth(ship);
			ys = gObj_GetHeight(ship);
			xb = gObj_GetWidth(base);

			if ((ship->x + xs / 2 == base->x + xb / 2) &&
			    (ship->y + ys == base->y)) {
				player_attached = 1;
				screen_bridge = 1;
				PlaySoundEffect(SND_CONTACT);
			}
		}
	}

	// exit if attach mode is ON
	if (player_attached == 1 ||
	    UpdateFuel() == 1 ||
	    ship->state == SH_DEAD) {
		return;
	}

	if (GKeys[KEY_RIGHT] == 1) {
		if (FacingRight(ship)) {
			if (IsTouch(ship->x + 2, ship->y, ship) == 0) {
				ship->x += 2;
			} else {
				if (ship->x == MaxRightPos(ship) && ChangeScreen(F_RIGHT) == 1) {
					ship->x = 0;
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
			if (IsTouch(ship->x - 2, ship->y, ship) == 0) {
				ship->x -= 2;
			} else {
				if (ship->x == 0 && ChangeScreen(F_LEFT) == 1) {
					ship->x = MaxRightPos(ship);
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

	if (GKeys[KEY_UP] == 1) {
		if ((ship->y & 1) != 0)
			dy = 1;
		else
			dy = 2;

		if (IsTouch(ship->x, ship->y - dy, ship) == 0) {
			ship->y -= dy;
		} else {
			if (ship->y == 0 && ChangeScreen(F_UP) == 1) {
				ship->y = MaxBottomPos(ship);
				InitNewScreen();
			}
		}
	} else {
		if (GKeys[KEY_DOWN] == 1) {
			if ((ship->y & 1) != 0)
				dy = 1;
			else
				dy = 2;

			if (IsTouch(ship->x, ship->y + dy, ship) == 0) {
				ship->y += dy;
			} else {
				if (ship->y == MaxBottomPos(ship) && ChangeScreen(F_DOWN) == 1) {
					ship->y = 0;
					InitNewScreen();
				}
			}
		} else {
			if (fallflag == 0) {
				if (IsTouch(ship->x, ship->y + 1, ship) == 0)
					ship->y += 1;

				fallflag = 1;
				if (ship->y == MaxBottomPos(ship) && ChangeScreen(F_DOWN) == 1) {
					ship->y = 0;
					InitNewScreen();
				}
			} else {
				fallflag -= 1;
			}
		}
	}

}

void ReEnableBase()
{
	TSHIP *base = gObj_Base();

	/* if exploded previously, reenable base at level start */
	if (base->ai_type == AI_EXPLOSION) {
		base_cur_screen = base_level_start;
		base->x = 160;
		base->y = 104;
		base->i = 1;
		base->min_frame = 0;
		base->cur_frame = 0;
		base->max_frame = 1;
		base->anim_speed = 0;
		base->anim_speed_cnt = 0;
		gObj_Constructor(base, AI_BASE);
	}

	if(base_cur_screen != ship_cur_screen) {
		base->state = SH_DEAD;
	} else {
		base->state = SH_ACTIVE;
	}
}

void Update_Base(TSHIP *base)
{
	TSHIP *ship = gObj_Ship();

	// do smth if attach mode ON
	int playMoveSound = 0;

	if (player_attached == 1 && ship->ai_type != AI_EXPLOSION) {
		if (GKeys[KEY_RIGHT] == 1) {
			if (FacingRight(ship)) {
				base->cur_frame ^= 1;
				playMoveSound = 1;

				if (IsTouch(ship->x + 2, ship->y, ship) + IsTouch(base->x + 2, base->y, base) == 0) {
					if (GetTileI((base->x + 40) >> 3, (base->y + 16) >> 3)) {
						ship->x += 2;
						base->x += 2;
					}
				} else {
					if (base->x == 280 && ChangeScreen(F_RIGHT) == 1) {
						ship->x = ShipBaseOffset();
						base->x = 0;
						base_cur_screen = ship_cur_screen;
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

				if (IsTouch(ship->x - 2, ship->y, ship) + IsTouch(base->x - 2, base->y, base) == 0) {
					if (GetTileI((base->x - 2) >> 3, (base->y + 16) >> 3)) {
						ship->x -= 2;
						base->x -= 2;
					}
				} else {
					if (base->x == 0 && ChangeScreen(F_LEFT) == 1) {
						//xxx
						ship->x = 280 + ShipBaseOffset();
						base->x = 280;
						base_cur_screen = ship_cur_screen;
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
			if (elevator_flag == 1)
				return;

			StopSoundEffect(SND_MOVE);

			player_attached = 0;
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
	TSHIP *ship = gObj_Ship();

	if (gobj != ship &&
	    gobj != gObj_Base() &&
	    gobj->ai_type != AI_BULLET &&
	    gobj->ai_type != AI_SHOT &&
	    gobj->ai_type != AI_BFG_SHOT &&
	    gobj->ai_type != AI_HOMING_SHOT &&
	    gobj->ai_type != AI_BONUS &&
	    gobj->ai_type != AI_ELECTRIC_SPARKLE_VERTICAL &&
	    gobj->ai_type != AI_ELECTRIC_SPARKLE_HORIZONTAL) {
		// /2 less points for hard mode, /2 less for rocket launcher of BFG
		int score = gobj->ai_type * 100 * (game_level & 7) + (RandomInt() & 127);

		if (game->easy_mode)
			score >>= 1;

		if (ship->i == SHIP_TYPE_ROCKET_LAUNCHER || ship->i == SHIP_TYPE_BFG)
			score >>= 1;

		AddScore(score);
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

void DoMachineGun()
{
	TSHIP *ship = gObj_Ship();
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
			// Create a new bullet.
			//if ship is not facing right or left, then exit.
			if (!FacingLeft(ship) && !FacingRight(ship))
				return;

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
			bullet->just_created = 1;
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

void DoRocketLauncher()
{
	static int rl_timeout = 0;
	if (--rl_timeout < 0)
		rl_timeout = 0;

	if (GKeys[KEY_FIRE] == 1) {
		if (!rl_timeout) {
			// Create a new bullet.
			TSHIP *ship = gObj_Ship();

			//if ship is not facing right or left, then exit.
			if (!FacingLeft(ship) && !FacingRight(ship))
				return;

			TSHIP *bullet = gObj_CreateObject();
			bullet->i = 54;
			bullet->y = ship->y + 1;
			bullet->dy = 0;
			bullet->anim_speed = 4;
			bullet->anim_speed_cnt = bullet->anim_speed;
			bullet->move_speed_cnt = bullet->move_speed;
			gObj_Constructor(bullet, AI_HOMING_SHOT);
			bullet->just_created = 1;
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
	TSHIP *ship = gObj_Ship(); /* FIXME: Should be gObj_CreateObject */
	TSHIP *base = gObj_Base(); /* FIXME: Should be gObj_CreateObject */

	GarageRestore();

	/* Erase all game objects and init object queue */
	gObj_DestroyAll();

	// base data
	base->state = SH_ACTIVE;
	base->x = 148;
	base->y = 104;
	base->i = 1;
	base->min_frame = 0;
	base->cur_frame = 0;
	base->max_frame = 1;
	base->anim_speed = 0;
	base->anim_speed_cnt = 0;
	gObj_Constructor(base, AI_BASE);

	// flying ship data
	ship->state = SH_ACTIVE;
	ship->i = GetPlayerShipIndex();
	ship->x = 148 + ShipBaseOffset();
	ship->y = 68;

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

	ship->cur_frame = ((game_level & 1) == 0 ? ship->max_frame : ship->min_frame);
	ship->anim_speed = 1;
	ship->anim_speed_cnt = 1;
	gObj_Constructor(ship, AI_SHIP);
}

void InitEnemies()
{
	ROOM *room = game->world->room + ship_cur_screen;
	OBJECT *object = room->object;
	int count = room->object_num;

	/* FIXME: Clear all except ship/base and smoke */
	TSHIP *gobj = gObj_First(2);
	for (; gobj; gobj = gObj_Next(gobj)) {
		if (gobj->ai_type != AI_SMOKE)
			gObj_DestroyObject(gobj);
	}

	screen_procedure = room->procedure;
	cur_screen_bonus = room->bonus;

	for (; count > 0; count--, object++) {

		if (object->index == BONUS_FACEBOOK || object->index == BONUS_TWITTER) {
			if (!sn_enabled || game->mode == GM_DEMO || base_cur_screen < ship_cur_screen)
				continue;
		}

		TSHIP *en = gObj_CreateObject();
		en->i = object->index;
		en->x = object->x << 2;
		en->y = object->y;
		en->anim_speed = object->speed * 2;
		en->anim_speed_cnt = object->speed * 2;
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

			if (hidden_level_entered) {
				DestroyHiddenAreaAccess(en, 0);
			}
		}
	}
}

void InitNewScreen()
{
	UnpackLevel(game->world, ship_cur_screen);

	InitEnemies();

	ReEnableBase();

	laser_dir = 0;

	if (ship_cur_screen == 92) {
		// Memorize that we entered the hidden area once.
		hidden_level_entered = 1;
	} else if (hidden_level_entered && ship_cur_screen == 1) {
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

	ship_cur_screen = GAME_START_SCREEN;
	base_cur_screen = GAME_START_SCREEN;
	base_level_start = GAME_START_SCREEN;
	game_level = GameLevelFromScreen(GAME_START_SCREEN);

	player_attached = 0;
	hidden_level_entered = 0;

	InitGaragesForNewGame();
	InitShip();
	InitNewScreen();

	for (int f = 0; f <= 6; f++) {
		PutStream(0, STATUSBAR1[f * 43 + 1] * 8, &STATUSBAR1[f * 43 + 2]);
	}
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

	player_attached = 0;
	ship_cur_screen = base_level_start;
	base_cur_screen = base_level_start;

	InitShip();
	InitNewScreen();
}

void BlitStatus()
{
	static char string_buffer[16];

	// level
	sprintf(string_buffer, "%02d", game_level);
	PutString(8*16, 8*20, &string_buffer[0]);

	// fuel
	sprintf(string_buffer, "%04d", game->fuel);
	PutString(8*14, 8*21, &string_buffer[0]);

	// score
	sprintf(string_buffer, "%08d", game->score);
	PutString(8*10, 8*22, &string_buffer[0]);

	// health bar.
	string_buffer[2] = 0;
	for (int y = 0; y < 3; ++y) {
		string_buffer[1] = string_buffer[0] = (game->health > y) ? 84 : 88;
		PutStream(8*19, 8*(22 - y), (unsigned char *)&string_buffer[0]);
	}

	// laser
	BlitLaserStatus();

	// lives
	sprintf(string_buffer, "%02d", game->lives);
	PutString(8*28, 8*21, &string_buffer[0]);

	// score record
	PutString(8*22, 8*22, "88888888");

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
	dst.y = 142;
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
	dst.h = (small_screen->h - title->h) / 2;

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
		ship_cur_screen = 0;
		title_start_flag = 1;
		InitNewScreen();
		BlitLevel(ship_cur_screen);
		PutSpriteI(50*4, 108, 45, 0);
		PutString(76, 88, "ESPACIO PARA COMENZAR");
		PutString(60, 24, "ORIGINAL GAME: PEDRO RUIZ");
		PutString(76, 36, "REMAKE: DMITRY SMAGIN");
		PutString(140, 44, "ALEXEY PAVLOV");
		PutString(60, 56, "MUSIC AND SFX: MARK BRAGA");
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

		// Update animations and screen.
		GKeys[KEY_RIGHT] = (ship->x < 93) ? 1 : 0;
		GKeys[KEY_LEFT] = 0;
		GKeys[KEY_UP] = (ship->x < 93 && ship->y > 40 ) ? 1 : 0;
		GKeys[KEY_DOWN] = 0;
		GKeys[KEY_FIRE] = 0;

		Update_Ship(ship);

		TSHIP *gobj = gObj_First(2);
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
	for (TSHIP *gobj = gObj_First(0); gobj; gobj = gObj_Next(gobj)) {
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
	unsigned int shadow = (world->room + ship_cur_screen)->shadow;

	for (TSHIP *gobj = gObj_First(0); gobj; gobj = gObj_Next(gobj)) {
		/* Blit shadow only if object needs it */
		if (gobj->flags & GOBJ_SHADOW)
			PutSpriteS(gobj->x, gobj->y, gobj->i, gobj->cur_frame, shadow);
	}
}

void RenderGame(int renderStatus)
{
	if (modern_background) {
		BlitBackground(game->world, ship_cur_screen); // blit background
		BlitLevelOutlines(game->world, ship_cur_screen);
		BlitEnemyOutlines(game->world); // draw moving objects' outlines (shadows)
	} else {
		EraseBackground(0);
	}

	BlitLevel(ship_cur_screen); // blit walls
	BlitBfg();
	BlitEnemies(); // draw all enemies and cannon+base

	if (renderStatus)
		BlitStatus(); // draw score etc
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
			sprintf(screen_nr_text, "SCREEN %i", ship_cur_screen);
			PutString(8*17, 8*17, screen_nr_text);
		}
#endif
		RecordDemo();

		if (Keys[SC_ESCAPE] == 1) {
			SetGameMode(GM_TITLE);
			LM_ResetKeys();
			break;
		}

		// win the game:
		if (screen_procedure == 3 /*&& base_cur_screen >= 70*/) {
			SetGameMode(GM_YOUWIN);

			LM_ResetKeys();
			PublishScore();
			return;
		}

		// do enemies
		for (TSHIP *gobj = gObj_First(0); gobj; gobj = gObj_Next(gobj))
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

void EnableSocialNetworkIcon(int enable)
{
	sn_enabled = enable;
}

void SetModernBackground(int imodern)
{
	modern_background = imodern;
}

int CurrentLevel()
{
	return game_level;
}

int CurrentPoints()
{
	return game->score;
}

void LoadGame(TGAMEDATA *data)
{
	player_attached = 0;
	screen_bridge = 0;
	ResetLaser();
	SetGameMode(GM_GAME);
	elevator_flag = 0;

	game->easy_mode = data->easy_mode;

	hidden_level_entered = data->hidden_level_entered;
	game->fuel = data->fuel;
	game->lives = data->lives;
	game->health = data->health;
	game->score = data->score;

	ship_cur_screen = data->base_level;
	base_cur_screen = data->base_level;
	base_level_start = data->base_level;
	game_level = GameLevelFromScreen(data->base_level);

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
	data->base_level = base_level_start;
	data->lives = game->lives;
	data->hidden_level_entered = hidden_level_entered;
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
	player_attached = 0;
	base_level_start = 1;
	screen_bridge = 0;
	game_level = 1;
	game->fuel = 5000;
	game->lives = 10;
	hidden_level_entered = 0;
	game->health = 3;
	game->score = 0;
	ResetLaser();
	SetGameMode(gameMode);
	elevator_flag = 0;

	InitNewGame();
}
