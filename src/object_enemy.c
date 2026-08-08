#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "data.h"
#include "demo.h"
#include "random.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "engine.h"
#include "object.h"
#include "object_enemy.h"
#include "object_garage.h"
#include "room.h"

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

		if (gObj_CheckTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
			gobj->x += gobj->dx;
		else
			gobj->ai_update_cnt = 0;

		if (gObj_CheckTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
			gobj->y += gobj->dy;
		else
			gobj->ai_update_cnt = 0;
	}
}

void Update_Kamikaze(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

	if (ship == NULL)
		return;

	if (ship->i == SHIP_TYPE_OBSERVER) {
		Update_Random(gobj);
		return;
	}

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

		if (gObj_CheckTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
			gobj->x += gobj->dx;
		else
			gobj->ai_update_cnt = 0;

		if (gObj_CheckTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
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
		if (gObj_CheckTouch(gobj->x, gobj->y + gobj->dy, gobj) == 0)
			gobj->y += gobj->dy;
		else
			gobj->dy = -gobj->dy;
	}
}

void Update_CeilingCannon(TSHIP *gobj)
{
	// if object is spawned - do nothing
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
		gObj_Constructor(j, AI_KAMIKADZE);
		j->parent = gobj;
		PlaySoundEffect(SND_CANNON_SHOOT);
	}
}

void Update_HomingMissile(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

	if (ship == NULL)
		return;

	UpdateAnimation(gobj);

	if (gobj->x > 0) {
		gobj->x -= 2;
		gObj_CheckTouch(gobj->x, gobj->y, gobj);
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

	if (ship == NULL)
		return;

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
			gObj_Constructor(bullet, AI_BULLET);
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
		if (gObj_CheckTouch(gobj->x + gobj->dx, gobj->y, gobj) == 0)
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

/* FIXME: this should belong to ship code */
void Create_Smoke(TSHIP *gobj)
{
	if (gobj->smoke)
		return;

	TSHIP *smoke = gObj_CreateObject();
	gobj->smoke = smoke;
	smoke->i = 46;
	smoke->x = gobj->x + 8;
	smoke->y = gobj->y - 8;
	smoke->dy = -1;
	smoke->dx = FacingRight(gobj) ? -1 : (FacingLeft(gobj) ? 1 : 0);
	smoke->anim_speed = 4;
	smoke->anim_speed_cnt = smoke->anim_speed;
	smoke->cur_frame = 0;
	smoke->max_frame = 4;
	gObj_Constructor(smoke, AI_SMOKE);
	smoke->parent = gobj;
}

void Destroy_Smoke(TSHIP *gobj)
{
	if (!gobj->smoke)
		return;

	gObj_DestroyObject(gobj->smoke);
	gobj->smoke = NULL;
}

void Update_Smoke(TSHIP *gobj)
{
	if (UpdateAnimation(gobj) == 1) {
		gobj->x = gobj->parent->x + 8;
		gobj->y = gobj->parent->y - 8;
		gobj->cur_frame = 0;
	} else if (gobj->cur_frame % 2) {
		gobj->x += gobj->dx;
		gobj->y += gobj->dy;
	}
}

void Update_Explosion(TSHIP *gobj)
{
	if (UpdateAnimation(gobj) == 1) {
		if (gobj->bonus_type) {
			if (gobj->regenerate_bonus) {
				// Hit with laser, restore the other bonus.
				gObj_Constructor(gobj, AI_BONUS);
				switch (gobj->bonus_type) {
				case BONUS_FACEBOOK:
					gobj->i = BONUS_TWITTER;
					break;
				case BONUS_TWITTER:
					gobj->i = BONUS_FACEBOOK;
					break;
				default:
					gobj->i = gobj->bonus_type;
					break;
				}

				gobj->bonus_type = 0;
				gobj->regenerate_bonus = 0;
				gobj->dx = 0;
				gobj->dy = 0;
				gobj->max_frame = 0;
				gobj->cur_frame = 0;
				gobj->min_frame = 0;
				gobj->anim_speed = 4;
				return;
			} else {
				PlaySoundEffect(SND_BONUS);

				switch (gobj->bonus_type) {
				case BONUS_HP:
					// add HP
					++game->health;
					if (game->health > 3)
						game->health = 3;

					if (game->health > 1)
						Destroy_Smoke(gObj_Ship());

					break;

				case BONUS_FACEBOOK:
				case BONUS_TWITTER:
					//HitTheBonus(gobj->bonus_type);
					break;
				}
			}
		}

		/* Check if player exploded and restart level */
		if (gobj->restart_level) {
			RestartLevel();
		} else {
			gObj_DestroyObject(gobj);
		}

		return;
	}
}

void Update_Bridge(TSHIP *gobj)
{
	if (game->player_attached) {
		gobj->flags |= GOBJ_SOLID | GOBJ_SHADOW | GOBJ_VISIBLE;
	} else {
		gobj->flags &= ~(GOBJ_SOLID | GOBJ_SHADOW | GOBJ_VISIBLE);
	}

	int a = game->player_attached ? 245 : 0;

	// seal or unseal the floor
	for (int f = 0; f <= 4; f++) {
		SetTileI((gobj->x >> 3) + f, gobj->y >> 3, a);
	}

}

void Update_Bullet(TSHIP *gobj)
{
	// random exploding
	if (gobj->parent)
	if (gobj->y + 16 < gobj->parent->y && (RandomInt() & 63) == 1) {
		gObj_Explode(gobj);
		return;
	}

	gobj->x += gobj->dx;
	if (gobj->x < 0 || gobj->x > SCREEN_WIDTH) {
		gObj_Explode(gobj);
		return;
	}

	gobj->y += gobj->dy;
	if (gobj->y < 0)
		gObj_Explode(gobj);

}

void Update_HomingShot(TSHIP *gobj)
{
	if (gObj_CheckTouch(gobj->x, gobj->y, gobj)) {
		gObj_Explode(gobj);
		return;
	}

	// Calculate vertical speed.
	gobj->dy = 0;

	if (++gobj->ticks_passed > 10) {
		TSHIP *trg = gObj_First();
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
}

void Update_Shot(TSHIP *gobj)
{
	if (gObj_CheckTouch(gobj->x, gobj->y, gobj)) {
		gObj_Explode(gobj);
		return;
	}

	gobj->x += gobj->dx;
	gobj->y += gobj->dy;
}

void Update_Elevator(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

	if (ship == NULL)
		return;

	TSHIP *base = ship->base;

	if (game->player_attached == 1) {
		// start to lift only when ship and base are standing on the elevator
		// ugly, improve in future
		//if ((gobj->x == 256 && base->x >= 260) || (gobj->x == 16 && base->x <= 20))
		if (gobj->x == base->x - 4) {
			static int el_phase = 0;

			game->elevator_flag = 1;

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

					int sy = gObj_GetHeight(ship);

					ship->y = 112 - sy;
					base->y = 112;

					ChangeScreen(F_UP);
					game->base_screen = game->ship_screen;
					InitNewScreen();

					// now i is invalid, because InitNewScreen reenables enemies
					// spawn new elevator
					gobj = gObj_CreateObject();
					gobj->i = 21;
					gobj->x = base->x - 4;
					gobj->y = 128;
					gObj_Constructor(gobj, AI_ELEVATOR);
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

					if (game->ship_screen != 69) {
						game->level += 1;
						GarageSave();
						//PublishScore();
						//GameLevelUp();
					}
					game->base_restart_screen = game->ship_screen;

					// destroy elevator or it will roll forever
					// but if not screen 69
					if (game->base_screen != 69) {
						TSHIP *lift = gObj_First();
						for (; lift; lift = gObj_Next(lift)) {
							if(lift->ai_type == AI_ELEVATOR)
								gObj_DestroyObject(lift);
						}
					}

					game->elevator_flag = 0;
					game->health = 3;

					goto _here;
				}
			}

			ship->y -= 1;
			base->y -= 1;
			gobj->y -= 1;

		_here:
			if (game->elevator_flag)
				PlaySoundEffect(SND_ELEVATOR);
			else
				StopSoundEffect(SND_ELEVATOR);
		}
	}
}
