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
	[AI_LASER]			= 0,
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
	/* FIXME: if uncommented, it makes base not appear at the level start */
	//memset(obj, 0, sizeof(TSHIP));
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
