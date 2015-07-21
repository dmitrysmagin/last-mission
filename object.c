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
#include "engine.h"
#include "room.h"

static int EnemyFlags[] = {
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
	obj->state = SH_DEAD;
}

/* FIXME: Later remove the parameter */
void gObj_DestroyAll(int i)
{
	memset(&Ships[i], 0, sizeof(TSHIP) * (SHIPS_NUMBER - i));
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
static int UpdateAnimation(TSHIP *gobj)
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
static int UpdateMoveSpeed(TSHIP *gobj)
{
	if (gobj->move_speed_cnt == 0) {
		gobj->move_speed_cnt = gobj->move_speed;
		return 1;
	} else {
		gobj->move_speed_cnt -= 1;
	}

	return 0;
}

/* FIXME: Move to header file */
int IsTouch(int x, int y, TSHIP *gobj);

void Update_Static(TSHIP *gobj)
{
	UpdateAnimation(gobj);
}

void Update_Random(TSHIP *gobj)
{
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

		if (IsTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
			gobj->x += gobj->dx;
		else
			gobj->ai_update_cnt = 0;

		if (IsTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
			gobj->y += gobj->dy;
		else
			gobj->ai_update_cnt = 0;
	}
}

void Update_Kamikaze(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

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

		if (IsTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
			gobj->x += gobj->dx;
		else
			gobj->ai_update_cnt = 0;

		if (IsTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
			gobj->y += gobj->dy;
		else
			gobj->ai_update_cnt = 0;
	}
}

void Update_SparkleVertical(TSHIP *gobj)
{
	UpdateAnimation(gobj);
	if (UpdateMoveSpeed(gobj) == 1) {
		if (gobj->dy == 0)
			gobj->dy = 1;
		if (IsTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
			gobj->y += gobj->dy;
		else
			gobj->dy = -gobj->dy;
	}
}

void Update_CeilingCannon(TSHIP *gobj)
{
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
}

void Update_HomingMissile(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

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
}

void Update_Cannon(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

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
}

void Update_SparkleHorizontal(TSHIP *gobj)
{
	UpdateAnimation(gobj);
	if (UpdateMoveSpeed(gobj) == 1) {
		if (gobj->dx == 0)
			gobj->dx = 1;
		if (IsTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
			gobj->x += gobj->dx;
		else
			gobj->dx = -gobj->dx;
	}
}

void Update_Bonus(TSHIP *gobj)
{
	if (UpdateAnimation(gobj) == 1) {
		static int yOffset[] = {
			-1, -1, -2, -2, -1, 1, 1, 2, 2, 1
		};

		gobj->y += yOffset[gobj->dy];
		gobj->dy = (gobj->dy + 1) % (sizeof(yOffset) / sizeof(yOffset[0]));
	}
}

void Update_Smoke(TSHIP *gobj)
{
	if (UpdateAnimation(gobj) == 1) {
		gObj_DestroyObject(gobj);
	} else if (gobj->cur_frame % 2) {
		gobj->x += gobj->dx;
		gobj->y += gobj->dy;
	}
}

/* HACK: later remove this */
void RestartLevel();
extern TGAMEDATA *game;

void Update_Explosion(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

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
					//HitTheBonus(gobj->explosion.bonus_type);
					break;
				}
			}
		}

		gObj_DestroyObject(gobj);
		if (gobj == ship)
			RestartLevel();

		return;
	}
}

/* HACK: later remove this */
extern int player_attached;

void Update_Bridge(TSHIP *gobj)
{
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

}
/* HACK: later remove this */
void BlowUpEnemy(TSHIP *gobj);

void Update_Bullet(TSHIP *gobj)
{
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

}

void Update_HomingShot(TSHIP *gobj)
{
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
}

void Update_Shot(TSHIP *gobj)
{
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
}
