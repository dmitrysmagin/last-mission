#include <stdio.h>
#include <stdlib.h>

#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "object.h"
#include "object_laser.h"
#include "engine.h"
#include "room.h"

int laser_overload = 0;
int laser_dir = 0;

static int x_start, x_end;

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

		xs2 = gobj->x + gObj_GetWidth(gobj);
		ys2 = gobj->y + gObj_GetHeight(gobj);

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

static int laser_phase = 0;
static int previous_phase = 0;

// ugly procedure which animates laser
void DoLaser()
{
	TSHIP *ship = gObj_Ship();

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
				TSHIP *laser = gObj_CreateObject();
				gObj_Constructor(laser, AI_LASER);
				laser->parent = ship;
				laser->x = x_start = ship->x + 32;
				x_end = x_start;
				laser->dx = 0;
				laser->y = ship->y + 6;
				laser->dy = 1;
				laser_dir = 1;
				laser_phase = 0;
			// if facing left
			} else if (FacingLeft(ship)) {
				TSHIP *laser = gObj_CreateObject();
				gObj_Constructor(laser, AI_LASER);
				laser->parent = ship;
				laser->x = x_start = ship->x - 1;
				x_end = x_start;
				laser->dx = 0;
				laser->y = ship->y + 6;
				laser->dy = 1;
				laser_dir = -1;
				laser_phase = 0;
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

void BlitLaser(TSHIP *gobj)
{
	if (laser_dir != 0) {
		DrawLine(gobj->x, gobj->y,
			 gobj->x + gobj->dx, gobj->y, RGB(170, 170, 170));
	}
}

void BlitLaserStatus()
{
	for (int i = 0; i <= 31; i++) {

		unsigned int c = ((i < (laser_overload >> 3)) ? RGB(255, 0, 0) : 0);

		PutPixel(i + 192, 162, c);
		PutPixel(i + 192, 163, c);
		PutPixel(i + 192, 164, c);
	}
}

void ResetLaser()
{
	laser_overload = 0;
}

void Update_Laser(TSHIP *gobj)
{
	TSHIP *ship = gobj->parent;
	int dx;

	/* HACK: destroy laser if not our ship, otherwise weird behavior */
	if (ship->i != SHIP_TYPE_LASER) {
		laser_dir = 0;
		gObj_DestroyObject(gobj);
		return;
	}

	// animate laser
	// shooting right
	if (laser_dir == 1) {
		if (laser_phase == 0) {
			x_start = ship->x + 32;

			for (dx = 0; dx <= 11; dx++) {
				x_end += 1;
				if (IsLaserHit2(x_start, x_end, gobj->y) == 1) {
					laser_phase = 1;
					break;
				}
			}
		} else {
			for (dx = 0; dx <= 11; dx++) {
				x_start += 1;

				if (x_start == x_end) {
					laser_dir = 0;
					gObj_DestroyObject(gobj);
					break;
				}

				IsLaserHit2(x_start, x_end, gobj->y);
			}
		}

	} else { // shooting left
		if (laser_dir == -1) {
			if (laser_phase == 0) {
				x_start = ship->x - 1;

				for (dx = 0; dx <= 11; dx++) {
					x_end -= 1;

					if (IsLaserHit2(x_start, x_end, gobj->y) == 1) {
						laser_phase = 1;
						break;
					}
				}
			} else {
				for (dx = 0; dx <= 11; dx++) {
					x_start -= 1;
					if (x_start == x_end) {
						laser_dir = 0;
						gObj_DestroyObject(gobj);
						break;
					}

					IsLaserHit2(x_start, x_end, gobj->y);
				}
			}
		}
	}

	/* HACK: Fill x and y for gobj routines */
	if (x_start > x_end) {
		gobj->x = x_end;
		gobj->dx = x_start - x_end;
	} else {
		gobj->x = x_start;
		gobj->dx = x_end - x_start;
	}
}
