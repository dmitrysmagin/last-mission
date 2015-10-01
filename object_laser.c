#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <math.h>
//#include <SDL/SDL.h>
//#include <SDL/SDL_rotozoom.h>

//#include "m_data.h"
//#include "demo.h"
//#include "random.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "object.h"
//#include "object_enemy.h"
//#include "object_garage.h"
//#include "object_bfg.h"
#include "object_laser.h"
#include "engine.h"
#include "room.h"

int laser_overload = 0;
int laser_dir = 0;

static int x_start, x_end, ly;

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

void BlitLaser()
{
	if (laser_dir != 0) {
		DrawLine(x_start, ly, x_end, ly, RGB(170, 170, 170));
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