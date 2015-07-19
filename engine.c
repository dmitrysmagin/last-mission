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
#include "enemies.h"
#include "engine.h"
#include "room.h"

#define GAME_START_SCREEN 1 // Start of the labyrinth.

//#define GOD_MODE

int EnemyFlags[] = {
	[AI_STATIC]			= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_RANDOM_MOVE]		= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_KAMIKADZE]			= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_ELECTRIC_SPARKLE_VERTICAL]	=            GOBJ_HURTS|                         GOBJ_VISIBLE,
	[AI_CEILING_CANNON]		= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_HOMING_MISSLE]		=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_CANNON]			= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_ELECTRIC_SPARKLE_HORIZONTAL]=            GOBJ_HURTS|                         GOBJ_VISIBLE,
	[AI_EXPLOSION]			=                                                GOBJ_VISIBLE,
	[AI_BRIDGE]			= GOBJ_SOLID|                        GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_BULLET]			=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_ELEVATOR]			= GOBJ_SOLID|                        GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_SMOKE]			=                                                GOBJ_VISIBLE,
	[AI_BONUS]			= GOBJ_SOLID                                    |GOBJ_VISIBLE,
	[AI_SHOT]			=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_GARAGE]			= 0,
	[AI_SPARE_SHIP]			= GOBJ_SOLID           |GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_HOMING_SHOT]		=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_HIDDEN_AREA_ACCESS]		= GOBJ_SOLID           |GOBJ_DESTROY,
	[AI_BFG_SHOT]			=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_SHIP]			= GOBJ_SOLID           |GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_BASE]			= GOBJ_SOLID           |GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
};

unsigned char ChangeScreen(int flag);
int IsOverlap(int x, int y, TSHIP *gobj1, TSHIP *gobj2);
int IsTouch(int x, int y, TSHIP *gobj);
int UpdateFuel();
void DoShip();
void ReEnableBase();
void DoBase();
void BlowUpEnemy(TSHIP *gobj);
int UpdateLaser(int i);
void DoLaser();
void DoMachineGun();
void DoBFG();
void DoRocketLauncher();
int UpdateAnimation(TSHIP *gobj);
int UpdateMoveSpeed(TSHIP *gobj);
void DoEnemy(TSHIP *gobj);
void InitShip();
void InitEnemies();
void InitNewScreen();
void InitNewGame();
int UpdateLives();
void RestartLevel();
void BlitStatus();
void DoTitle();
void DoWinScreen();
void DoGame();
void CreateExplosion(TSHIP *gobj);
void CreateWallExplosion(int x, int y);
void RenderGame(int renderStatus);
void InitGaragesForNewGame();
void SetGarageShipIndex(int garageId, int shipIndex);
int GarageShipIndex(int garageId);
int GetPlayerShipIndex();

// Game callbacks.
void HitTheBonus(int);
void PublishScore();
void GameLevelUp();

unsigned char ship_cur_screen = 0;

// Actual garage data, valid for current game.
int garage_data[MAX_GARAGES][2];
// Garage data of the last game start. Will be restored if player is dead.
int main_garage_data[MAX_GARAGES][2];

typedef struct {
	int xc, yc;
	int xt, yt;
	TSHIP *ship;
	int hit_now;
	int hit_count;
} TBFGTARGET;

#define BFG_KILL_DISTANCE 60
#define BFG_KILL_TIME 16
#define MAX_BFG_TARGETS 13
TBFGTARGET BfgTargets[MAX_BFG_TARGETS];
int bfg_on = 0;

void BestPositionInGarage(TSHIP *ship, int *x, int *y);

unsigned char player_attached = 0;
int base_cur_screen;
int base_level_start = 1;
char screen_procedure;
int screen_bridge = 0;
int game_level = 1;
int ticks_for_damage = 0;
int laser_overload = 0;
int sn_enabled = 1; // Facebook/Twitter integration.
int elevator_flag = 0; // 1 if elevator is working
int frame_skip = 0;
int modern_background = 1;
int title_start_flag = 0;
int youwin_start_flag = 0;
int cur_screen_bonus = 0;
int hidden_level_entered = 0;

#define F_UP 0
#define F_RIGHT 1
#define F_DOWN 2
#define F_LEFT 3

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

void GetCurrentSpriteDimensions(TSHIP *i, int *cx, int *cy)
{
	if (i->ai_type == AI_GARAGE) {
		*cx = GARAGE_WIDTH;
		*cy = GARAGE_HEIGHT;
	} else if (i->ai_type == AI_HIDDEN_AREA_ACCESS) {
		*cx = i->dx;
		*cy = i->dy;
	} else {
		*cx = GetSpriteW(i->i);
		*cy = GetSpriteH(i->i);
	}
}

int ShipBaseOffset()
{
	int xs, ys, xb, yb;

	GetCurrentSpriteDimensions(gObj_Ship(), &xs, &ys);
	GetCurrentSpriteDimensions(gObj_Base(), &xb, &yb);
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
	int x, y;

	GetCurrentSpriteDimensions(i, &x, &y);
	return SCREEN_WIDTH - x;
}

int MaxBottomPos(TSHIP *i)
{
	int x, y;
	GetCurrentSpriteDimensions(i, &x, &y);
	return ACTION_SCREEN_HEIGHT - y;
}

void CleanupBfg()
{
	memset(BfgTargets, 0, sizeof(BfgTargets));
	bfg_on = 0;
}

int IsOverlap(int x, int y, TSHIP *gobj1, TSHIP *gobj2)
{
	int xs, ys, xs2, ys2;

	GetCurrentSpriteDimensions(gobj1, &xs, &ys);
	GetCurrentSpriteDimensions(gobj2, &xs2, &ys2);

	ys += y;
	xs += x;

	ys2 += gobj2->y;
	xs2 += gobj2->x;

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

/* Check if gobj1 could destroy gobj2 */
void HandleShipsContact(TSHIP *gobj1, TSHIP *gobj2)
{
	int i, j; /* FIXME: 1 if controlled object */

	i = (gobj1 == gObj_Ship() || gobj1 == gObj_Base()) ? 1 : 0;
	j = (gobj2 == gObj_Ship() || gobj2 == gObj_Base()) ? 1 : 0;

	// i == 1, j == 1
	/* Both objects are player controlled. */
	if (i && j)
		return;

	// i == 0, j == 0
	/* Both objects are NPC. */
	if (!i && !j) {
		if (gobj1->ai_type != AI_SHOT &&
		    gobj2->ai_type != AI_SHOT &&
		    gobj1->ai_type != AI_HOMING_SHOT &&
		    gobj2->ai_type != AI_HOMING_SHOT &&
		    gobj1->ai_type != AI_BFG_SHOT &&
		    gobj2->ai_type != AI_BFG_SHOT) {
			return;
		}
	}

	if ((gobj1->ai_type == AI_SPARE_SHIP && j) || // j == 1
		(gobj2->ai_type == AI_SPARE_SHIP && i)) { // i == 1
		return;
	}

	if (gobj1->ai_type == AI_BFG_SHOT) {
		BlowUpEnemy(gobj2);
	} else if (gobj2->ai_type == AI_BFG_SHOT) {
		BlowUpEnemy(gobj1);
	} else if (gobj1->ai_type == AI_BONUS) {
		/* Bonus hit by a bullet. Swap bonus. */
		if (!j) { // j == 0
			gobj1->explosion.regenerate_bonus = 1;
		}

		BlowUpEnemy(gobj1);
	} else if (gobj2->ai_type == AI_BONUS) {
		/* Bonus hit by a bullet. Swap bonus. */
		if (!i) { // i == 0
			gobj1->explosion.regenerate_bonus = 1;
		}

		BlowUpEnemy(gobj2);
	} else if (!(gobj1->flags & GOBJ_HURTS) && !(gobj2->flags & GOBJ_HURTS)) {
		return;
	} else {
		if (!j || game->easy_mode || i) // i == 1, j == 0
			BlowUpEnemy(gobj1);

		if (!i || game->easy_mode || j) // i == 0, j == 1
			BlowUpEnemy(gobj2);
	}
}

int IsTouch(int x, int y, TSHIP *gobj)
{
	int xs, ys;

	if (x < 0 || y < 0)
		return 1;

	GetCurrentSpriteDimensions(gobj, &xs, &ys);

	ys += y;
	xs += x;

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

		if (IsOverlap(x, y, gobj, en)) {
			HandleShipsContact(gobj, en);

			if (!(en->flags & GOBJ_SOLID))
				continue;

			if (!(gobj->flags & GOBJ_SOLID))
				continue;

			/* This is to make BFG shot multiple enemies */
			if (gobj->ai_type == AI_BFG_SHOT)
				return 0;

			return 1;
		}
	}

	for (int dy = y; dy < ys; dy++) {
		for (int dx = x; dx < xs; dx++) {
			unsigned char b = GetTileI(dx >> 3, dy >> 3);

			if (b == 0)
				continue;

			// All tiles are solid.
			if (gobj == gObj_Ship() && b >= 246) {
				// ship dies from touching certain tiles
				BlowUpEnemy(gobj);
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

void DoShip()
{
	TSHIP *ship = gObj_Ship();
	TSHIP *base = gObj_Base();
	static unsigned char fallflag = 1;
	static int dy;

	if (--ticks_for_damage < 0)
		ticks_for_damage = 0;


	if (ship->ai_type == AI_EXPLOSION) {
		DoEnemy(ship);

		return;
	}

	// check if ships are attached
	if (base->state == SH_ACTIVE &&
	    base->ai_type != AI_EXPLOSION &&
	    player_attached == 0) {
		if (FacingLeft(ship) || FacingRight(ship)) {
			int xs, ys, xb, yb;

			GetCurrentSpriteDimensions(ship, &xs, &ys);
			GetCurrentSpriteDimensions(base, &xb, &yb);

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
		base->ai_type = AI_BASE;
		base->flags = EnemyFlags[AI_BASE];
	}

	if(base_cur_screen != ship_cur_screen) {
		base->state = SH_DEAD;
	} else {
		base->state = SH_ACTIVE;
	}
}

void DoBase()
{
	TSHIP *ship = gObj_Ship();
	TSHIP *base = gObj_Base();

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

void BlowUpEnemy(TSHIP *gobj)
{
#ifdef GOD_MODE
	if (gobj == gObj_Ship() || gobj == gObj_Base()) {
		// You cannot hurt a god.
		return;
	}
#endif

	if (gobj == gObj_Ship()) {
		// This is the player ship.
		if (game->easy_mode) {
			if (!ticks_for_damage) {
				ticks_for_damage = 20;
				--game->health;
			}
		} else {
			game->health = -1;
		}

		if (game->health >= 0) {
			return;
		}
	}

	if (gobj == gObj_Base()) {
		// This is a base, do not kill the base while moving up,
		// otherwise player will be disappointed + there is a bug whith
		// elevator_flag being not reset.
		if (elevator_flag)
			return;
	}

	/* Exit if non-killable enemy */
	if (!(gobj->flags & GOBJ_DESTROY) && gobj->ai_type != AI_BONUS) /* HACK for AI_BONUS */
		return;

	if (gobj->ai_type == AI_SHOT || gobj->ai_type == AI_HOMING_SHOT) {
		gObj_DestroyObject(gobj);

		return;
	}

	if (gobj->ai_type == AI_BFG_SHOT) {
		CleanupBfg();
		gObj_DestroyObject(gobj);

		return;
	}

	if (gobj->ai_type == AI_HIDDEN_AREA_ACCESS) {
		DestroyHiddenAreaAccess(gobj, 1);

		return;
	}

	// some corrections for homing missiles
	if (gobj->i == 40 || gobj->i == 41) {
		if (game->easy_mode) {
			CreateExplosion(gobj);
			gobj->x += SCREEN_WIDTH;
			PlaySoundEffect(SND_EXPLODE);
		}

		return;
	}

	// update score with the killed ship data.
	UpdateScoreWithShip(gobj);

	// if blowing base - zero player_attached
	if (gobj == gObj_Base())
		player_attached = 0;

	// reactivate parent cannon
	if (gobj->i == 34 && gobj->parent)
		gobj->parent->dx = 0;

	// special procedures for breakable walls
	if (gobj->i == 6) {
		if (gobj->cur_frame == 0) {
			gobj->x -= 8;
			SetTileI(gobj->x >> 3, gobj->y >> 3, 0);
			SetTileI(gobj->x >> 3, (gobj->y >> 3) + 1, 0);
		} else {
			SetTileI((gobj->x >> 3) + 1, gobj->y >> 3, 0);
			SetTileI((gobj->x >> 3) + 1, (gobj->y >> 3) + 1, 0);
		}
	}

	if (gobj->ai_type == AI_BONUS) {
		// Memorize bonus type.
		gobj->explosion.bonus_type = gobj->i;
	}

	if (game->easy_mode &&
	    (gobj->ai_type == AI_RANDOM_MOVE || gobj->ai_type == AI_KAMIKADZE)) {
		// Generate bonus if defined by screen and if
		// this ship is the last one on the screen.
		if (cur_screen_bonus) {
			int alive_ship = 0;
			TSHIP *last_alive = gObj_First(2);

			for (; last_alive; last_alive = gObj_Next(last_alive)) {
				if (last_alive != gobj &&
				    (last_alive->ai_type == AI_RANDOM_MOVE ||
				     last_alive->ai_type == AI_KAMIKADZE)) {
					alive_ship = 1;
					break;
				}
			}

			if (!alive_ship) {
				gobj->explosion.bonus_type = cur_screen_bonus;
				gobj->explosion.regenerate_bonus = 1;
			}
		}
	}

	gobj->ai_type = AI_EXPLOSION;
	gobj->flags = EnemyFlags[AI_EXPLOSION];

	if (gobj->i == 6) {
		gobj->i = 7;
	} else {
		if (gobj->i == 1) {
			// explosion's sprite is smaller than base's thus a little hack
			gobj->i = 23;
			gobj->x += 4;
			gobj->y += 4;
		} else {
			if(gobj->i == 0)
				gobj->i = 23;
			else
				gobj->i = 2;
		}
	}

	gobj->min_frame = 0;
	gobj->max_frame = 2;
	gobj->cur_frame = 0;
	gobj->anim_speed = 6;
	gobj->anim_speed_cnt = 6;

	PlaySoundEffect(SND_EXPLODE);
}

int IsLaserHit2(int x_start, int x_end, int y)
{
	int xs2, ys2, return_value = 0;

	// swap if x_start > x_end
	if (x_start > x_end) {
		x_start ^= x_end;
		x_end ^= x_start;
		x_start ^= x_end;
	}

	if (x_start <= 0)
		return_value = 1;

	if (x_end >= 319)
		return_value =  1;

	if (GetTileI(x_start >> 3, y >> 3))
		return_value = 1;

	if (GetTileI(x_end >> 3, y >> 3))
		return_value = 1;

	TSHIP *gobj = gObj_First(1);
	for (; gobj; gobj = gObj_Next(gobj)) {
		/* exclude non-hittable objects */
		if (!(gobj->flags & GOBJ_SOLID))
			continue;

		int cx, cy;
		GetCurrentSpriteDimensions(gobj, &cx, &cy);

		xs2 = gobj->x + cx;
		ys2 = gobj->y + cy;

		if (y >= gobj->y && y < ys2) {
			if (x_start < gobj->x && x_end >= xs2) {
				if (gobj->ai_type == AI_BONUS) {
					gobj->explosion.regenerate_bonus = 1;
				}

				BlowUpEnemy(gobj);
				continue;
			}

			for (int dx = x_start; dx <= x_end; dx++) {
				if (dx >= gobj->x && dx < xs2) {
					if (gobj->ai_type == AI_BONUS) {
						gobj->explosion.regenerate_bonus = 1;
					}

					BlowUpEnemy(gobj);
					return 1;
				}
			}
		}
	}

	return return_value;
}

int UpdateLaser(int i)
{
	laser_overload += i;

	if (laser_overload > 32*8-1) {
		laser_overload = 0;
		return 1;
	}

	if (laser_overload < 0)
		laser_overload = 0;

	return 0;
}

void DoMachineGun()
{
	TSHIP *ship = gObj_Ship();
	static int mg_timeout = 0;
	if (--mg_timeout < 0)
		mg_timeout = 0;

	if (GKeys[KEY_FIRE] == 1) {
		if (UpdateLaser(1) == 1) {
			BlowUpEnemy(ship);
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
			bullet->ai_type = AI_SHOT;
			bullet->flags = EnemyFlags[AI_SHOT];
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

void DoBFG()
{
	static int mg_timeout = 0;
	if (--mg_timeout < 0)
		mg_timeout = 0;

	if (GKeys[KEY_FIRE] == 1) {
		if (!mg_timeout && !bfg_on) {
			// Create a new bullet.
			TSHIP *ship = gObj_Ship();

			//if ship is not facing right or left, then exit.
			if (!FacingLeft(ship) && !FacingRight(ship))
				return;

			TSHIP *bullet = gObj_CreateObject();
			bullet->i = 56;
			bullet->x = ship->x + (FacingRight(ship) ? 16 : -11);
			bullet->y = ship->y + 5;
			bullet->dy = 0;
			bullet->dx = FacingRight(ship) ? 2 : -2;
			bullet->anim_speed = 10;
			bullet->anim_speed_cnt = bullet->anim_speed;
			bullet->move_speed_cnt = bullet->move_speed;
			bullet->cur_frame = 0;
			bullet->max_frame = 3;
			bullet->ai_type = AI_BFG_SHOT;
			bullet->flags = EnemyFlags[AI_BFG_SHOT];
			bullet->just_created = 1;
			bullet->parent = ship;

			bfg_on = 1;

			// Reset timeout.
			mg_timeout = 20;

			PlaySoundEffect(SND_CANNON_SHOOT);
		}
	} else {
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
			bullet->ai_type = AI_HOMING_SHOT;
			bullet->flags = EnemyFlags[AI_HOMING_SHOT];
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


int laser_dir = 0;
static int x_start, x_end, ly;

// ugly procedure which animates laser
void DoLaser()
{
	TSHIP *ship = gObj_Ship();
	static int laser_phase = 0, dx;
	static int previous_phase = 0;

	int fireOn = (GKeys[KEY_FIRE] == 1) && (ship->i == SHIP_TYPE_LASER);
	if (fireOn) {
		if(UpdateLaser(1) == 1) {BlowUpEnemy(ship); LM_ResetKeys(); return;}
	} else {
		UpdateLaser(-1);
	}

	// if zero - no shooting, 1 - shooting right, -1 - shooting left
	if (laser_dir == 0) {
		if (fireOn && elevator_flag == 0) { // HACK, or you will shoot your base
			//if ship facing right
			if (FacingRight(ship)) {
				x_start = ship->x + 32;
				x_end = x_start;
				ly = ship->y + 6;
				laser_dir = 1;
				laser_phase = 0;
				goto __woo;
			} else { // if facing left
				if (FacingLeft(ship)) {
					x_start = ship->x - 1;
					x_end = x_start;
					ly = ship->y + 6;
					laser_dir = -1;
					laser_phase = 0;
					goto __woo;
				}
			}
		}
	} else {
		__woo:
		// animate laser
		// shooting right
		if (laser_dir == 1) {
			// another dirty hack or laser will overlap with ship when moving
			//x_start = ship->x + 32;

			if (laser_phase == 0) {
				x_start = ship->x + 32;

				for (dx = 0; dx <= 11; dx++) {
					x_end += 1;
					if (IsLaserHit2(x_start, x_end, ly) == 1) {
						laser_phase = 1;
						break;
					}
				}
			} else {
				for (dx = 0; dx <= 11; dx++) {
					x_start += 1;

					if (x_start == x_end) {
						laser_dir = 0;
						break;
					}

					IsLaserHit2(x_start, x_end, ly);
				}
			}

		} else { // shooting left
			// another dirty hack or laser will overlap with ship when moving
			//x_start = ship->x - 1;

			if (laser_dir == -1) {
				if (laser_phase == 0) {
					x_start = ship->x - 1;

					for (dx = 0; dx <= 11; dx++) {
						x_end -= 1;

						if (IsLaserHit2(x_start, x_end, ly) == 1) {
							laser_phase = 1;
							break;
						}
					}
				} else {
					for (dx = 0; dx <= 11; dx++) {
						x_start -= 1;
						if (x_start == x_end) {
							laser_dir = 0;
							break;
						}

						IsLaserHit2(x_start, x_end, ly);
					}
				}
			}
		}
	}

	if (fireOn) {
		if (laser_phase != previous_phase && !laser_phase) {
			PlaySoundEffect(SND_LASER_SHOOT);
		}

		previous_phase = laser_phase;
	} else {
		previous_phase = 1;
	}
}

// update animation counters
// returns 1 if reached the end of animation cycle
int UpdateAnimation(TSHIP *gobj)
{
	// do animation counters
	if (gobj->anim_speed_cnt == 0) {
		gobj->anim_speed_cnt = gobj->anim_speed;
		gobj->cur_frame += 1;
		if (gobj->cur_frame > gobj->max_frame) {
			gobj->cur_frame = gobj->min_frame;
			return 1;
		}
	} else {
		gobj->anim_speed_cnt -= 1;
	}

	return 0;
}

// returns 1 if reached the end of move-wait cycle
int UpdateMoveSpeed(TSHIP *gobj)
{
	if (gobj->move_speed_cnt == 0) {
		gobj->move_speed_cnt = gobj->move_speed;
		return 1;
	} else {
		gobj->move_speed_cnt -= 1;
	}

	return 0;
}

void CreateExplosion(TSHIP *gobj)
{
	TSHIP *j = gObj_CreateObject();
	j->i = 2;
	j->x = gobj->x;
	j->y = gobj->y;
	j->anim_speed = 6;
	j->anim_speed_cnt = 6;
	j->max_frame = 2;
	j->ai_type = AI_EXPLOSION;
	j->flags = EnemyFlags[AI_EXPLOSION];
}

int ShipsDistance(TSHIP *i, TSHIP *j)
{
	int x = i->x - j->x;
	int y = i->y - j->y;

	return (int)sqrt((float)(x*x + y*y));
}

void AddBfgTarget(TSHIP *gobj, TSHIP *bfg)
{
	TBFGTARGET *t = NULL;
	for (int n = 0; n < MAX_BFG_TARGETS; ++n) {
		if (BfgTargets[n].ship == gobj) {
			t = &BfgTargets[n];
			break;
		} else if (!t && !BfgTargets[n].ship) {
			t = &BfgTargets[n];
		}
	}

	if (!t)
		return;

	++t->hit_count;
	t->hit_now = 1;
	t->ship = gobj;

	t->xc = bfg->x + 5;
	t->yc = bfg->y + 5;
	t->xt = gobj->x + 8;
	t->yt = gobj->y + 8;
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
	j->ai_type = AI_EXPLOSION;
	j->flags = EnemyFlags[AI_EXPLOSION];
}

void DoSmoke()
{
	if (!game->easy_mode || game->health > 1)
		return;

	static int smoke_counter = 0;
	if (--smoke_counter > 0)
		return;

	smoke_counter = 60;

	TSHIP *i = gObj_Ship();
	TSHIP *j = gObj_CreateObject();
	j->i = 46;
	j->x = i->x + 8;
	j->y = i->y - 8;
	j->dy = -1;
	j->dx =
		FacingRight(i) ? -1 :
		(FacingLeft(i) ? 1 : 0);
	j->anim_speed = 4;
	j->anim_speed_cnt = j->anim_speed;
	j->max_frame = 4;
	j->ai_type = AI_SMOKE;
	j->flags = EnemyFlags[AI_SMOKE];
}

void DoEnemy(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();
	TSHIP *base = gObj_Base();

	// if main ship is exploding, freeze other enemies except bridge and garage
	// it's not safe but ship dies after all and enemy data is reinitialized then
	if (gobj != ship && ship->ai_type == AI_EXPLOSION) {
		if(gobj->ai_type != AI_BRIDGE &&
		   gobj->ai_type != AI_GARAGE &&
		   gobj->ai_type != AI_HIDDEN_AREA_ACCESS &&
		   gobj->ai_type != AI_SPARE_SHIP) {
			gobj->ai_type = AI_STATIC; // don't affect ship itself
		}
	}

	// do different ai types
	switch (gobj->ai_type) {
	case AI_SHIP:
		DoShip();
		break;
	case AI_BASE:
		DoBase();
		break;
	case AI_STATIC: // breakable wall or non-moving enemy
		UpdateAnimation(gobj);
		break;

	case AI_RANDOM_MOVE:
_random_move_ai:
		UpdateAnimation(gobj);
		if (UpdateMoveSpeed(gobj) == 1) {
			if (gobj->ai_update_cnt == 0) {
				gobj->dx = RandomInt() & 3;
				if (gobj->dx >= 2)
					gobj->dx = -1;

				gobj->dy = RandomInt() & 3;
				if (gobj->dy >= 2)
					gobj->dy = -1;

				gobj->ai_update_cnt = RandomInt() & 0x1f;
				if(gobj->ai_update_cnt < 15) gobj->ai_update_cnt = 15;
			} else {
				gobj->ai_update_cnt -= 1;
			}
		_optimize1:
			if (IsTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
				gobj->x += gobj->dx;
			else
				gobj->ai_update_cnt = 0;

			if (IsTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
				gobj->y += gobj->dy;
			else
				gobj->ai_update_cnt = 0;

		}
		break;

	case AI_KAMIKADZE:
		if (ship->i == SHIP_TYPE_OBSERVER)
			goto _random_move_ai;

		UpdateAnimation(gobj);
		if (UpdateMoveSpeed(gobj) == 1) {
			if (gobj->ai_update_cnt == 0) {
				if (gobj->x > ship->x) {
					gobj->dx = -1;
				} else {
					if (gobj->x < ship->x)
						gobj->dx = 1;
					else
						gobj->dx = 0;
				}

				if (gobj->y > ship->y) {
					gobj->dy = -1;
				} else {
					if (gobj->y < ship->y)
						gobj->dy = 1;
					else
						gobj->dy = 0;
				}

				gobj->ai_update_cnt = 15;
			} else {
				gobj->ai_update_cnt -= 1;
			}

			goto _optimize1;
		}
		break;

	case AI_ELECTRIC_SPARKLE_VERTICAL:
		UpdateAnimation(gobj);
		if (UpdateMoveSpeed(gobj) == 1) {
			if (gobj->dy == 0)
				gobj->dy = 1;
			if (IsTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
				gobj->y += gobj->dy;
			else
				gobj->dy = -gobj->dy;
		}
		break;

	case AI_CEILING_CANNON: // ceiling cannon spawning kamikazes
		// if object is spawned - do nothig
		if (gobj->dx == 1)
			return;

		if (UpdateAnimation(gobj) == 1) {
			gobj->dx = 1;

			// spawn a new enemy
			TSHIP *j = gObj_CreateObject();
			j->i = 34;
			j->x = gobj->x;
			j->y = gobj->y + 16;
			j->anim_speed = 4;
			j->anim_speed_cnt = j->anim_speed;
			j->max_frame = 3;
			j->ai_type = AI_KAMIKADZE;
			j->flags = EnemyFlags[AI_KAMIKADZE];
			j->parent = gobj;
			PlaySoundEffect(SND_CANNON_SHOOT);
		}
		break;
	case AI_HOMING_MISSLE:
		UpdateAnimation(gobj);

		if (gobj->x > 0) {
			gobj->x -= 2;
			IsTouch(gobj->x, gobj->y, gobj);
			if (gobj->x < ship->x)
				return;

			if (gobj->y > ship->y)
				gobj->y -= 1;

			if (gobj->y < ship->y && gobj->y < 63) {
				if ((RandomInt() & 1) == 1)
					gobj->y += 1;
			}
		} else {
			gobj->x = 296;
			gobj->y = RandomInt() & 63;
			if (gobj->i == 40)
				gobj->i = 41;
			else
				gobj->i = 40;
		}
		break;

	case AI_CANNON:
		if (gobj->x - 40 > ship->x) {
			gobj->cur_frame = 0;
		} else {
			if (gobj->x + 40 < ship->x)
				gobj->cur_frame = 2;
			else
				gobj->cur_frame = 1;
		}

		if ((RandomInt() & 255) > 252) {
			TSHIP *bullet = gObj_CreateObject();
			if (bullet) {
				bullet->i = 43;
				if (gobj->cur_frame == 0) {
					bullet->x = gobj->x - 4;
					bullet->y = gobj->y + 4;
					bullet->dx = -1;
					bullet->dy = -1;
				}
				if (gobj->cur_frame == 1) {
					bullet->x = gobj->x + 6;
					bullet->y = gobj->y - 4;
					bullet->dx = 0;
					bullet->dy = -1;
				}
				if (gobj->cur_frame == 2) {
					bullet->x = gobj->x + 16;
					bullet->y = gobj->y + 4;
					bullet->dx = 1;
					bullet->dy = -1;
				}
				bullet->anim_speed = 4;
				bullet->anim_speed_cnt = bullet->anim_speed;
				bullet->ai_type = AI_BULLET;
				bullet->flags = EnemyFlags[AI_BULLET];
				bullet->parent = gobj;
			}
		}
		break;

	case AI_ELECTRIC_SPARKLE_HORIZONTAL:
		UpdateAnimation(gobj);
		if (UpdateMoveSpeed(gobj) == 1) {
			if (gobj->dx == 0)
				gobj->dx = 1;
			if (IsTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
				gobj->x += gobj->dx;
			else
				gobj->dx = -gobj->dx;
		}
		break;

	case AI_BONUS:
		if (UpdateAnimation(gobj) == 1) {
			static int yOffset[] = {
				-1, -1, -2, -2, -1, 1, 1, 2, 2, 1
			};

			gobj->y += yOffset[gobj->dy];
			gobj->dy = (gobj->dy + 1) % (sizeof(yOffset) / sizeof(yOffset[0]));
		}
		break;

	case AI_SMOKE:
		if (UpdateAnimation(gobj) == 1) {
			gObj_DestroyObject(gobj);
		} else if (gobj->cur_frame % 2) {
			gobj->x += gobj->dx;
			gobj->y += gobj->dy;
		}
		break;

	case AI_EXPLOSION: // explosion, do one animation cycle and deactivate enemy entry
		if (UpdateAnimation(gobj) == 1) {
			if (gobj->explosion.bonus_type) {
				if (gobj->explosion.regenerate_bonus) {
					// Hit with laser, restore the other bonus.
					gobj->ai_type = AI_BONUS;
					gobj->flags = EnemyFlags[AI_BONUS];
					switch (gobj->explosion.bonus_type) {
					case BONUS_FACEBOOK:
						gobj->i = BONUS_TWITTER;
						break;
					case BONUS_TWITTER:
						gobj->i = BONUS_FACEBOOK;
						break;
					default:
						gobj->i = gobj->explosion.bonus_type;
						break;
					}

					gobj->explosion.bonus_type = 0;
					gobj->explosion.regenerate_bonus = 0;
					gobj->dx = 0;
					gobj->dy = 0;
					gobj->max_frame = 0;
					gobj->cur_frame = 0;
					gobj->min_frame = 0;
					gobj->anim_speed = 4;
					return;
				} else {
					PlaySoundEffect(SND_BONUS);

					switch (gobj->explosion.bonus_type) {
					case BONUS_HP:
						// add HP
						++game->health;
						if (game->health > 3)
							game->health = 3;
						break;

					case BONUS_FACEBOOK:
					case BONUS_TWITTER:
						HitTheBonus(gobj->explosion.bonus_type);
						break;
					}
				}
			}

			gObj_DestroyObject(gobj);
			if (gobj == ship)
				RestartLevel();

			return;
		}
		break;

	case AI_BRIDGE: // bridge, appear if bonded ship, disappear otherwise
		if (player_attached) {
			gobj->flags |= GOBJ_SOLID | GOBJ_SHADOW | GOBJ_VISIBLE;
		} else {
			gobj->flags &= ~(GOBJ_SOLID | GOBJ_SHADOW | GOBJ_VISIBLE);
		}

		int a = player_attached ? 245 : 0;

		// seal or unseal the floor
		for (int f = 0; f <= 4; f++) {
			SetTileI((gobj->x >> 3) + f, gobj->y >> 3, a);
		}
		break;

	case AI_GARAGE:
		if (ship->ai_type == AI_EXPLOSION)
			break;

		if (GarageShipIndex(gobj->i) != -1) {
			break;
		}

		int w, h;
		GetCurrentSpriteDimensions(ship, &w, &h);

		if (!gobj->garage_inactive &&
		    (ship->x >= gobj->x) &&
		    (ship->x + w < gobj->x + GARAGE_WIDTH) &&
		    (ship->y >= gobj->y) &&
		    (ship->y + h < gobj->y + GARAGE_HEIGHT)) {
			// Player ship is inside the garage, lets
			// change the ship if possible.
			TSHIP *spare = gObj_First(2);

			for (; spare; spare = gObj_Next(spare)) {
				if (spare->ai_type == AI_SPARE_SHIP)
					break;
			}

			if (spare) {
				// Swap data of the player ship and
				// the spare ship.
				/* FIXME: Rework this ugly code */
				TSHIP tmp;
				tmp = *ship;
				*ship = *spare;
				*spare = tmp;

				TSHIP *garage = ship->garage;

				garage->garage_inactive = 1;

				SetGarageShipIndex(gobj->i, spare->i);
				SetGarageShipIndex(garage->i, -1);

				ship->ai_type = AI_SHIP;
				ship->flags = EnemyFlags[AI_SHIP];
				ship->garage = NULL;

				spare->ai_type = AI_SPARE_SHIP;
				spare->flags = EnemyFlags[AI_SPARE_SHIP];
				spare->garage = gobj;

				// restore HP
				game->health = 3;

				PlaySoundEffect(SND_CONTACT);
			}
		} else if (gobj->garage_inactive) {
			if (!IsOverlap(ship->x, ship->y, ship, gobj)) {
				gobj->garage_inactive = 0;
			}
		}
		break;

	case AI_SPARE_SHIP:
		{
			const int speed = 1;
			int x, y;
			BestPositionInGarage(gobj, &x, &y);

			if (x > gobj->x) {
				gobj->x += speed;
			} else {
				if (x < gobj->x)
					gobj->x -= speed;
			}

			if (y > gobj->y) {
				gobj->y += speed;
			} else {
				if (y < gobj->y)
					gobj->y -= speed;
			}

			int middle_frame = (gobj->max_frame + gobj->min_frame) / 2;

			if (gobj->cur_frame > gobj->min_frame && gobj->cur_frame <= middle_frame)
				--(gobj->cur_frame);
			else
				if (gobj->cur_frame > middle_frame && gobj->cur_frame < gobj->max_frame)
					++(gobj->cur_frame);
		}
		break;

	case AI_BULLET: // bullet
		// random exploding
		if (gobj->parent)
		if (gobj->y + 16 < gobj->parent->y && (RandomInt() & 63) == 1) {
			BlowUpEnemy(gobj);
			return;
		}

		gobj->x += gobj->dx;
		if (gobj->x < 0 || gobj->x > SCREEN_WIDTH) {
			BlowUpEnemy(gobj);
			return;
		}

		gobj->y += gobj->dy;
		if (gobj->y < 0)
			BlowUpEnemy(gobj);
		break;

	case AI_HOMING_SHOT: // missle shot by player.
		if (gobj->just_created == 1) {
			if (IsTouch(gobj->x, gobj->y, gobj)) {
				BlowUpEnemy(gobj);
				return;
			}
		}

		// Calculate vertical speed.
		gobj->dy = 0;

		if (++gobj->ticks_passed > 10) {
			TSHIP *trg = gObj_First(2);
			TSHIP *best = NULL;
			int dx_best = 0;

			for (; trg; trg = gObj_Next(trg)) {
				if (trg->ai_type != AI_RANDOM_MOVE &&
				    trg->ai_type != AI_KAMIKADZE)
					continue;

				if (trg->i == 11)
					continue;

				int dx = trg->x - gobj->x;

				if (gobj->cur_frame < 2) {
					// moving left.
					if (dx > 10 || dx < -110)
						continue;
					if (!best || dx > dx_best) {
						best = trg;
						dx_best = dx;
					}
				} else {
					// moving right.
					if (dx < 10 || dx > 110)
						continue;
					if (!best || dx > dx_best) {
						best = trg;
						dx_best = dx;
					}
				}
			}

			if (best) {
				int dy = best->y - gobj->y;

				if (dy < 0)
					gobj->dy = -1;
				if (dy > 0)
					gobj->dy = 1;
			}
		}

		UpdateAnimation(gobj);

		gobj->x += gobj->dx;
		gobj->y += gobj->dy;

		if (/*gobj->x + gobj->dx < 0 ||
		    gobj->x + gobj->dx >= SCREEN_WIDTH ||
		    gobj->y < 0 ||
		    gobj->y >= SCREEN_HEIGHT ||*/
		    IsTouch(gobj->x, gobj->y, gobj)) {
			gObj_DestroyObject(gobj);
			return;
		}

		break;

	case AI_BFG_SHOT:
		UpdateAnimation(gobj);

		if (gobj->dx == 2)
			gobj->dx = 3;
		else if (gobj->dx == -2)
			gobj->dx = -3;
		else if (gobj->dx == 3)
			gobj->dx = 2;
		else if (gobj->dx == -3)
			gobj->dx = -2;

		// Calculate hit objects.
		for (int n = 0; n < MAX_BFG_TARGETS; ++n)
			BfgTargets[n].hit_now = 0;

		TSHIP *trg = gObj_First(2);
		for (; trg; trg = gObj_Next(trg)) {
			if (trg->ai_type == AI_KAMIKADZE ||
			    trg->ai_type == AI_RANDOM_MOVE) {
				if (ShipsDistance(gobj, trg) < BFG_KILL_DISTANCE) {
					AddBfgTarget(trg, gobj);
				}
			}
		}

		for (int n = 0; n < MAX_BFG_TARGETS; ++n) {
			if (BfgTargets[n].hit_count) {
				if (BfgTargets[n].hit_now) {
					if (BfgTargets[n].hit_count > BFG_KILL_TIME) {
						BlowUpEnemy(BfgTargets[n].ship);
					}
				} else {
					memset(BfgTargets + n, 0, sizeof(TBFGTARGET));
				}
			}
		}

	case AI_SHOT: // bullet shot by a player
		if (gobj->just_created) {
			gobj->just_created = 0;
			if (IsTouch(gobj->x, gobj->y, gobj)) {
				BlowUpEnemy(gobj);
				return;
			}
		}

		gobj->x += gobj->dx;
		gobj->y += gobj->dy;

		if (gobj->x + gobj->dx < 0 ||
		    gobj->x + gobj->dx >= SCREEN_WIDTH ||
		    gobj->y < 0 ||
		    gobj->y >= SCREEN_HEIGHT ||
		    IsTouch(gobj->x, gobj->y, gobj)) {
			BlowUpEnemy(gobj);
			return;
		}
		break;

	case AI_ELEVATOR: // elevator
		if (player_attached == 1) {
			// start to lift only when ship and base are standing on the elevator
			// ugly, improve in future
			//if ((gobj->x == 256 && base->x >= 260) || (gobj->x == 16 && base->x <= 20))
			if (gobj->x == base->x - 4) {
				static int el_phase = 0;

				elevator_flag = 1;

				if (el_phase == 0) {
					// when starting to lift up - unseal the floor
					// ugly, maybe change in future
					if (gobj->y == 120) {
						// unseal the floor
						for (int j = 0; j <= 5; j++) {
							SetTileI((gobj->x >> 3) + j, gobj->y >> 3, 0);
						}
					}

					// upper limit of the screen is reached
					if (ship->y == 0) {
						el_phase = 1;

						int sx, sy;
						GetCurrentSpriteDimensions(ship, &sx, &sy);

						ship->y = 112 - sy;
						base->y = 112;

						ChangeScreen(F_UP);
						base_cur_screen = ship_cur_screen;
						InitNewScreen();

						// now i is invalid, because InitNewScreen reenables enemies
						// spawn new elevator
						gobj = gObj_CreateObject();
						gobj->i = 21;
						gobj->x = base->x - 4;
						gobj->y = 128;
						gobj->ai_type = AI_ELEVATOR;
						gobj->flags = EnemyFlags[AI_ELEVATOR];
						goto _here;
					}
				} else {
					// if elevator is done lifting
					if (base->y == 104) {
						el_phase = 0;

						// seal the floor!
						for (int i = 0; i <= 5; i++) {
							SetTileI(((base->x - 4) >> 3) + i, (base->y + 16) >> 3, 245);
						}

						if (ship_cur_screen != 69) {
							game_level += 1;
							memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
							PublishScore();
							GameLevelUp();
						}
						base_level_start = ship_cur_screen;

						// destroy elevator or it will roll forever
						// but if not screen 69
						if (base_cur_screen != 69) {
							TSHIP *lift = gObj_First(2);
							for (; lift; lift = gObj_Next(lift)) {
								if(lift->ai_type == AI_ELEVATOR)
									gObj_DestroyObject(lift);
							}
						}

						elevator_flag = 0;
						game->health = 3;

						goto _here;
					}
				}

				ship->y -= 1;
				base->y -= 1;
				gobj->y -= 1;

			_here:
				if (elevator_flag)
					PlaySoundEffect(SND_ELEVATOR);
				else
					StopSoundEffect(SND_ELEVATOR);
			}
		}
		break;
	}
}

void InitShip()
{
	TSHIP *ship = gObj_Ship(); /* FIXME: Should be gObj_CreateObject */
	TSHIP *base = gObj_Base(); /* FIXME: Should be gObj_CreateObject */

	memcpy(garage_data, main_garage_data, sizeof(garage_data));

	/* FIXME: Clear all game objects and init object queue */
	gObj_DestroyAll(0);

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
	base->ai_type = AI_BASE;
	base->flags = EnemyFlags[AI_BASE];

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
	ship->ai_type = AI_SHIP;
	ship->flags = EnemyFlags[AI_SHIP];
}

void BestPositionInGarage(TSHIP *ship, int *x, int *y)
{
	int cxShip, cyShip;
	GetCurrentSpriteDimensions(ship, &cxShip, &cyShip);

	if (ship->garage == NULL) {
		// actually, should not happen.
		*x = ship->x;
		*y = ship->y;
	} else {
		TSHIP *garage = ship->garage;
		*x = garage->x + ((GARAGE_WIDTH - cxShip) >> 1);
		*y = garage->y + ((GARAGE_HEIGHT - cyShip) >> 1);
	}
}

void InitGaragesForNewGame()
{
	memset(garage_data, 0, sizeof(garage_data));

	int n = 0;
	garage_data[n  ][0] = 100;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 101;
	garage_data[n++][1] = SHIP_TYPE_MACHINE_GUN;

	garage_data[n  ][0] = 110;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 111;
	garage_data[n++][1] = SHIP_TYPE_ROCKET_LAUNCHER;

	garage_data[n  ][0] = 120;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 121;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 122;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 123;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 124;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 130;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 131;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 190;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 191;
	garage_data[n++][1] = SHIP_TYPE_BFG;

	memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
}

void SetGarageShipIndex(int garageId, int shipIndex)
{
	for (int i = 0; i < MAX_GARAGES; ++i) {
		if (garage_data[i][0] == garageId) {
			garage_data[i][1] = shipIndex;
		}
	}
}

int GarageShipIndex(int garageId)
{
	for (int i = 0; i < MAX_GARAGES; ++i) {
		if (garage_data[i][0] == garageId) {
			return garage_data[i][1];
		}
	}

	return -1;
}

int IsParked(int ship_type)
{
	for (int i = 0; i < MAX_GARAGES; ++i) {
		if (garage_data[i][0] && garage_data[i][1] == ship_type) {
			return 1;
		}
	}
	return 0;
}

int GetPlayerShipIndex()
{
	if (!IsParked(SHIP_TYPE_LASER))
		return SHIP_TYPE_LASER;

	if (!IsParked(SHIP_TYPE_MACHINE_GUN))
		return SHIP_TYPE_MACHINE_GUN;

	if (!IsParked(SHIP_TYPE_ROCKET_LAUNCHER))
		return SHIP_TYPE_ROCKET_LAUNCHER;

	if (!IsParked(SHIP_TYPE_BFG))
		return SHIP_TYPE_BFG;

	return SHIP_TYPE_OBSERVER;
}

void InitEnemies()
{
	ROOM *room = game->world->room + ship_cur_screen;
	OBJECT *object = room->object;
	int count = room->object_num;

	/* FIXME: Clear all except ship/base */
	gObj_DestroyAll(2);

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
		en->ai_type = object->ai;
		en->flags = EnemyFlags[object->ai];
		en->move_speed = 1; // standard
		en->move_speed_cnt = 1;

		/* FIXME: Special hack */
		if (en->i == 11 || en->i == 42)
			en->flags &= ~GOBJ_DESTROY;

		if (en->ai_type == AI_GARAGE) {
			en->i = object->garage_id;

			int iShip = GarageShipIndex(en->i);
			if (iShip != -1) {
				// Find which type of the ship is supposed to be here,
				// create the ship in the best position inside it.
				TSHIP *ship = gObj_CreateObject();
				ship->i = iShip;
				ship->ai_type = AI_SPARE_SHIP;
				ship->flags = EnemyFlags[AI_SPARE_SHIP];
				ship->garage = en;

				switch (iShip) {
				case SHIP_TYPE_LASER:
				case SHIP_TYPE_MACHINE_GUN:
				case SHIP_TYPE_ROCKET_LAUNCHER:
					ship->max_frame = 6;
					ship->min_frame = 0;
					break;
				case SHIP_TYPE_OBSERVER:
					ship->max_frame = 3;
					ship->min_frame = 1;
					ship->cur_frame = 1;
					break;
				case SHIP_TYPE_BFG:
					ship->max_frame = 4;
					ship->min_frame = 0;
					break;
				}
				BestPositionInGarage(ship, &ship->x, &ship->y);
			}
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
		memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
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
	for (int i = 0; i <= 31; i++) {

		unsigned int c = ((i < (laser_overload >> 3)) ? RGB(255, 0, 0) : 0);

		PutPixel(i + 192, 162, c);
		PutPixel(i + 192, 163, c);
		PutPixel(i + 192, 164, c);
	}

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

		DoShip();

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

void BlitLaser()
{
	if (laser_dir != 0) {
		DrawLine(x_start, ly, x_end, ly, RGB(170, 170, 170));
	}
}

void BlitBfg()
{
	if (bfg_on) {
		for (int n = 0; n < MAX_BFG_TARGETS; ++n) {
			if (BfgTargets[n].ship) {
				unsigned int color = RGB(85, 255, 85);

				if (BfgTargets[n].hit_count < BFG_KILL_TIME / 2)
					color = RGB(0, 170, 0);

				DrawLine(
					BfgTargets[n].xc,
					BfgTargets[n].yc,
					BfgTargets[n].xt,
					BfgTargets[n].yt,
					color);
			}
		}
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

unsigned char *pLightBuffer = 0;
int numLights = 0;
Light lights[MAX_LIGHTS];

void AddLight(int x, int y, int radius, unsigned char r, unsigned char g, unsigned char b)
{
	if (numLights >= MAX_LIGHTS)
		return;

	Light *l = &lights[numLights++];
	l->x = x;
	l->y = y;
	l->radius = radius;
	l->b = b;
	l->g = g;
	l->r = r;
}

int ExplosionRadius(TSHIP *ship, int maxRadius)
{
	return maxRadius * cos((3.14159 * 0.5 * ship->cur_frame) / ship->max_frame);
}

void CastLights()
{
	numLights = 0;

	// Laser.
	if (laser_dir) {
		AddLight((x_start + x_end) / 2, ly, 60, 50, 50, 50);
	}

	// Enemies.
	TSHIP *obj = gObj_First(0);
	for (; obj; obj = gObj_Next(obj)) {
		if (obj->ai_type != AI_BRIDGE) {
			switch (obj->ai_type) {
			case AI_ELECTRIC_SPARKLE_HORIZONTAL:
			case AI_ELECTRIC_SPARKLE_VERTICAL:
				AddLight(obj->x + 8, obj->y + 8, 50, 200, 50, 50);
				break;

			case AI_SHOT:
				AddLight(obj->x + 2, obj->y, 20, 100, 50, 50);
				break;

			case AI_BFG_SHOT:
				AddLight(obj->x + 5, obj->y + 5, 16, 90, 200, 90);
				break;

			case AI_HOMING_SHOT:
				if (obj->cur_frame < 2)
					AddLight(obj->x + 14, obj->y + 4, 20, 150, 110, 100);
				else
					AddLight(obj->x, obj->y + 4, 20, 150, 110, 100);
				break;

			case AI_EXPLOSION:
				AddLight(obj->x + 10, obj->y + 10, ExplosionRadius(obj, 80), 240, 100, 0);
				break;

			case AI_BONUS:
				if (obj->i == BONUS_HP) AddLight(obj->x + 5, obj->y + 5, 25, 200, 80, 80);
				break;

			case AI_SMOKE:
				AddLight(obj->x + 8, obj->y + 8, ExplosionRadius(obj, 40), 100, 70, 0);
				break;
			}
		}
	}
}

void BlitNonAmbientEnemies()
{
	for (TSHIP *gobj = gObj_First(0); gobj; gobj = gObj_Next(gobj)) {
		if (gobj->flags & GOBJ_SHADOW)
			PutSpriteI(gobj->x, gobj->y, gobj->i, gobj->cur_frame);
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
	BlitLaser(); // don't forget laser

	if (renderStatus)
		BlitStatus(); // draw score etc

	if (pLightBuffer) {
		// Render light map
#if 0
		/*
		 * TODO: Render to lights_screen and later mix with small_screen
		 * in LM_GFX_Flip
		 */
		BlitLevel();
		BlitNonAmbientEnemies();
		CastLights();
#endif
	}
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
			DoEnemy(gobj);

		DoSmoke();

		/* FIXME: Rework this later */
		TSHIP *ship = gObj_Ship();
		if (ship->state == SH_ACTIVE) {
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
				//DoLaser();
				break;
			}
		}

		/* FIXME: Make laser a valid game object */
		DoLaser();

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
	laser_overload = 0;
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

	memcpy(garage_data, data->garages, sizeof(garage_data));
	memcpy(main_garage_data, data->garages, sizeof(main_garage_data));

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
	memcpy(data->garages, main_garage_data, sizeof(main_garage_data));
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
	laser_overload = 0;
	SetGameMode(gameMode);
	elevator_flag = 0;

	InitNewGame();
}
