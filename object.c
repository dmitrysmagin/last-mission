#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "m_data.h"
#include "demo.h"
#include "random.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "object.h"
#include "object_bfg.h"
#include "object_enemy.h"
#include "object_garage.h"
#include "object_laser.h"
#include "engine.h"
#include "room.h"

static int EnemyFlags[] = {
	[AI_STATIC]			= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_RANDOM_MOVE]		= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_KAMIKADZE]			= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_ELECTRIC_SPARKLE_VERTICAL]	=            GOBJ_HURTS|                         GOBJ_VISIBLE,
	[AI_CEILING_CANNON]		= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_HOMING_MISSLE]		= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_CANNON]			= GOBJ_SOLID|GOBJ_HURTS|GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_ELECTRIC_SPARKLE_HORIZONTAL]=            GOBJ_HURTS|                         GOBJ_VISIBLE,
	[AI_EXPLOSION]			=                                                GOBJ_VISIBLE,
	[AI_BRIDGE]			= GOBJ_SOLID|                        GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_BULLET]			=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_ELEVATOR]			= GOBJ_SOLID|                        GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_SMOKE]			=                                                GOBJ_VISIBLE,
	[AI_BONUS]			= GOBJ_SOLID           |GOBJ_DESTROY            |GOBJ_VISIBLE,
	[AI_SHOT]			=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE|GOBJ_WEAPON,
	[AI_GARAGE]			= 0,
	[AI_SPARE_SHIP]			= GOBJ_SOLID           |GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE,
	[AI_HOMING_SHOT]		=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE|GOBJ_WEAPON,
	[AI_HIDDEN_AREA_ACCESS]		= GOBJ_SOLID           |GOBJ_DESTROY,
	[AI_BFG_SHOT]			=            GOBJ_HURTS|GOBJ_DESTROY            |GOBJ_VISIBLE|GOBJ_WEAPON,
	[AI_SHIP]			= GOBJ_SOLID           |GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE|GOBJ_PLAYER,
	[AI_BASE]			= GOBJ_SOLID           |GOBJ_DESTROY|GOBJ_SHADOW|GOBJ_VISIBLE|GOBJ_PLAYER,
	[AI_LASER]			=            GOBJ_HURTS                                      |GOBJ_WEAPON,
};

#define SHIPS_NUMBER 32
static TSHIP Ships[SHIPS_NUMBER];

TSHIP *gObj_Ship() { return &Ships[0]; }
TSHIP *gObj_Base() { return &Ships[1]; }

/* FIXME: Later remove the first parameter */
TSHIP *gObj_First(int i)
{
	TSHIP *obj = &Ships[i]; /* should be the start of gobj list */

	for (; obj < &Ships[SHIPS_NUMBER]; obj++) {
		if (obj->state == SH_ACTIVE)
			return obj;
	}

	return NULL;
}

TSHIP *gObj_Next(TSHIP *obj)
{
	for (; ++obj < &Ships[SHIPS_NUMBER]; ) {
		if (obj->state == SH_ACTIVE)
			return obj;
	}

	return NULL;
}

static int GetFreeEnemyIndex()
{
	for (int i = 2; i < SHIPS_NUMBER; i++) {
		if (Ships[i].state == SH_DEAD)
			return i; // and ai_type should be zero!
	}

	return SHIPS_NUMBER - 1;
}

TSHIP *gObj_CreateObject()
{
	TSHIP *obj = &Ships[GetFreeEnemyIndex()];
	memset(obj, 0, sizeof(TSHIP));

	obj->state = SH_ACTIVE;

	return obj;
}

void gObj_DestroyObject(TSHIP *obj)
{
	memset(obj, 0, sizeof(TSHIP));
	obj->state = SH_DEAD;
}

void gObj_DestroyAll()
{
	memset(Ships, 0, sizeof(TSHIP) * SHIPS_NUMBER);
}

/* FIXME: Perhaps, merge with gObj_CreateObject? */
void gObj_Constructor(TSHIP *gobj, int ai)
{
	gobj->ai_type = ai;
	gobj->flags = EnemyFlags[ai];

	/* FIXME: later add setting function pointers and calling them */
}

void gObj_Destructor(TSHIP *gobj)
{
}

/* update animation counters, return 1 if end of animation cycle */
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

/* returns 1 if end of move-wait cycle */
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

void gObj_Update(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

	/* FIXME: this should be done in ship destructor, move there later */
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
		Update_Ship(gobj);
		break;

	case AI_BASE:
		Update_Base(gobj);
		break;

	case AI_STATIC: // breakable wall or non-moving enemy
		Update_Static(gobj);
		break;

	case AI_RANDOM_MOVE:
		Update_Random(gobj);
		break;

	case AI_KAMIKADZE:
		Update_Kamikaze(gobj);
		break;

	case AI_ELECTRIC_SPARKLE_VERTICAL:
		Update_SparkleVertical(gobj);
		break;

	case AI_CEILING_CANNON: // ceiling cannon spawning kamikazes
		Update_CeilingCannon(gobj);
		break;

	case AI_HOMING_MISSLE:
		Update_HomingMissile(gobj);
		break;

	case AI_CANNON:
		Update_Cannon(gobj);
		break;

	case AI_ELECTRIC_SPARKLE_HORIZONTAL:
		Update_SparkleHorizontal(gobj);
		break;

	case AI_BONUS:
		Update_Bonus(gobj);
		break;

	case AI_SMOKE:
		Update_Smoke(gobj);
		break;

	case AI_EXPLOSION: // explosion, do one animation cycle and deactivate enemy entry
		Update_Explosion(gobj);
		break;

	case AI_BRIDGE: // bridge, appear if bonded ship, disappear otherwise
		Update_Bridge(gobj);
		break;

	case AI_GARAGE:
		Update_Garage(gobj);
		break;

	case AI_SPARE_SHIP:
		Update_SpareShip(gobj);
		break;

	case AI_BULLET: // bullet
		Update_Bullet(gobj);
		break;

	case AI_HOMING_SHOT: // missle shot by player.
		Update_HomingShot(gobj);
		break;

	case AI_BFG_SHOT:
		Update_BfgShot(gobj);
		break;

	case AI_SHOT: // bullet shot by a player
		Update_Shot(gobj);
		break;

	case AI_ELEVATOR: // elevator
		Update_Elevator(gobj);
		break;

	case AI_LASER: // laser
		Update_Laser(gobj);
		break;
	}
}

int gObj_GetWidth(TSHIP *gobj)
{
	switch (gobj->ai_type) {
	case AI_GARAGE:
		return GARAGE_WIDTH;
	case AI_HIDDEN_AREA_ACCESS:
	case AI_LASER:
		return gobj->dx;
	default:
		return GetSpriteW(gobj->i);
	}
}

int gObj_GetHeight(TSHIP *gobj)
{
	switch (gobj->ai_type) {
	case AI_GARAGE:
		return GARAGE_HEIGHT;
	case AI_HIDDEN_AREA_ACCESS:
	case AI_LASER:
		return gobj->dy;
	default:
		return GetSpriteH(gobj->i);
	}
}

void gObj_Explode(TSHIP *gobj)
{
#ifdef GOD_MODE
	if (gobj->ai_type == AI_SHIP || gobj->ai_type == AI_BASE) {
		// You cannot hurt a god.
		return;
	}
#endif

	/* Exit if non-killable enemy */
	if (!(gobj->flags & GOBJ_DESTROY))
		return;

	switch (gobj->ai_type) {
	case AI_SHIP:
		// This is the player ship.
		if (game->easy_mode) {
			if (!ticks_for_damage) {
				ticks_for_damage = 20;
				--game->health;
				if (game->health <= 1) {
					Create_Smoke(gobj);
				}
			}
		} else {
			game->health = -1;
		}

		if (game->health >= 0) {
			return;
		}

		gobj->restart_level = 1;
		break;

	case AI_BASE:
		// if blowing base - zero player_attached
		player_attached = 0;
		{
			/* Create new explosion object and hide base */
			TSHIP *exp = gObj_CreateObject();
			exp->i = 1;
			exp->x = gobj->x;
			exp->y = gobj->y;

			base_cur_screen = base_level_start;
			gobj->state = SH_HIDDEN;
			gobj->x = 160;
			gobj->y = 104;

			gobj = exp; /* HACKY: swap */
		}
		break;

	case AI_SHOT:
	case AI_HOMING_SHOT:
		gObj_DestroyObject(gobj);
		return;

	case AI_BFG_SHOT:
		CleanupBfg();
		gObj_DestroyObject(gobj);
		return;

	case AI_HIDDEN_AREA_ACCESS:
		DestroyHiddenAreaAccess(gobj, 1);
		return;

	case AI_BONUS:
		// Memorize bonus type.
		gobj->explosion.bonus_type = gobj->i;
		break;

	case AI_RANDOM_MOVE:
	case AI_KAMIKADZE:
		if (!game->easy_mode)
			break;

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

		// this may be spawned by ceiling cannon, reactivate it
		// FIXME: perhaps make it more general
		if (gobj->parent)
			gobj->parent->dx = 0;

		break;

	case AI_HOMING_MISSLE:
		{
			/* Create new explosion object and move missile outside
				the screen */
			TSHIP *exp = gObj_CreateObject();
			exp->i = 2;
			exp->x = gobj->x;
			exp->y = gobj->y;

			gobj->x += SCREEN_WIDTH;
			gobj = exp; /* HACKY: swap */
		}
		break;
	}

	// update score with the killed ship data.
	UpdateScoreWithShip(gobj);

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

	gObj_Constructor(gobj, AI_EXPLOSION);

	switch (gobj->i) {
	case 6: // special explosion for breakable walls
		gobj->i = 7;
		break;
	case 1: // explosion's sprite is smaller than base's thus a little hack
		gobj->x += 4;
		gobj->y += 4;
	case 0:  // ship
	case 51: // machine gun ship
	case 53: // rocket ship
		gobj->i = 23;
		break;
	default:
		gobj->i = 2;
		break;
	}

	gobj->min_frame = 0;
	gobj->max_frame = 2;
	gobj->cur_frame = 0;
	gobj->anim_speed = 6;
	gobj->anim_speed_cnt = 6;

	PlaySoundEffect(SND_EXPLODE);
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
static int gObj_CheckDestruction(TSHIP *gobj1, TSHIP *gobj2)
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

int gObj_CheckTouch(int x, int y, TSHIP *gobj)
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
