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

static int laser_overload = 0;
int laser_dir = 0;

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

/* FIXME: this should belong to ship code in fact */
void DoLaser()
{
	TSHIP *ship = gObj_Ship();

	if (GKeys[KEY_FIRE]) {
		/* Explode if laser overload */
		if (UpdateLaser(1)) {
			gObj_Explode(ship);
			LM_ResetKeys();
			return;
		}

		/* 0 - no shooting, 1 - shooting right, -1 - shooting left */
		if (laser_dir == 0) {
			if (!elevator_flag) { /* HACK: or you will shoot your base */
				//if ship facing right
				if (FacingRight(ship)) {
					TSHIP *laser = gObj_CreateObject();
					gObj_Constructor(laser, AI_LASER);
					laser->parent = ship;
					laser->x = ship->x + 32;
					laser->dx = 1;
					laser->y = ship->y + 6;
					laser->dy = 1;
					laser_dir = 1;
					laser_phase = 0;
					PlaySoundEffect(SND_LASER_SHOOT);
				// if facing left
				} else if (FacingLeft(ship)) {
					TSHIP *laser = gObj_CreateObject();
					gObj_Constructor(laser, AI_LASER);
					laser->parent = ship;
					laser->x = ship->x - 1;
					laser->dx = 1;
					laser->y = ship->y + 6;
					laser->dy = 1;
					laser_dir = -1;
					laser_phase = 0;
					PlaySoundEffect(SND_LASER_SHOOT);
				}
			}
		}
	} else {
		UpdateLaser(-1);
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
			gobj->x = ship->x + 32;

			for (dx = 0; dx <= 11; dx++) {
				gobj->dx++;
				if (IsTouch(gobj->x, gobj->y, gobj) == 1) {
					laser_phase = 1;
					break;
				}
			}
		} else {
			for (dx = 0; dx <= 11; dx++) {
				gobj->x++;
				gobj->dx--;

				if (gobj->dx <= 0) {
					laser_dir = 0;
					gObj_DestroyObject(gobj);
					break;
				}

				IsTouch(gobj->x, gobj->y, gobj);
			}
		}

	} else { // shooting left
		if (laser_dir == -1) {
			if (laser_phase == 0) {
				gobj->dx = ship->x - gobj->x - 1;

				for (dx = 0; dx <= 11; dx++) {
					gobj->x--;
					gobj->dx++;

					if (IsTouch(gobj->x, gobj->y, gobj) == 1) {
						laser_phase = 1;
						break;
					}
				}
			} else {
				for (dx = 0; dx <= 11; dx++) {
					gobj->dx--;
					if (gobj->dx <= 0) {
						laser_dir = 0;
						gObj_DestroyObject(gobj);
						break;
					}

					IsTouch(gobj->x, gobj->y, gobj);
				}
			}
		}
	}
}
