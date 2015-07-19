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
#include "enemies.h"
#include "engine.h"
#include "room.h"

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