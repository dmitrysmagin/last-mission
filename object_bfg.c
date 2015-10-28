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
#include "engine.h"
#include "object.h"
#include "object_bfg.h"
#include "object_enemy.h"
#include "room.h"

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
static TBFGTARGET BfgTargets[MAX_BFG_TARGETS];
static int bfg_on = 0;

int ShipsDistance(TSHIP *i, TSHIP *j)
{
	int x = i->x - j->x;
	int y = i->y - j->y;

	return (int)sqrt((float)(x*x + y*y));
}

void CleanupBfg()
{
	memset(BfgTargets, 0, sizeof(BfgTargets));
	bfg_on = 0;
}

void DoBFG(TSHIP *ship)
{
	static int mg_timeout = 0;
	if (--mg_timeout < 0)
		mg_timeout = 0;

	if (GKeys[KEY_FIRE] == 1) {
		if (!mg_timeout && !bfg_on && !elevator_flag) {
			//if ship is not facing right or left, then exit.
			if (!FacingLeft(ship) && !FacingRight(ship))
				return;

			// Create a new bullet.
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
			gObj_Constructor(bullet, AI_BFG_SHOT);
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

static void AddBfgTarget(TSHIP *gobj, TSHIP *bfg)
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

void Update_BfgShot(TSHIP *gobj)
{
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
					gObj_Explode(BfgTargets[n].ship);
				}
			} else {
				memset(BfgTargets + n, 0, sizeof(TBFGTARGET));
			}
		}
	}

	Update_Shot(gobj);
}
