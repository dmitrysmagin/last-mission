/*
	The Last Mission Remake

	Game engine, rewritten and reimagined :)
	Dmitry Smagin exmortis[REPLACE_WITH_AT_SYMBOL]yandex.ru

*/

#include <string.h>
#include "m_core.h"
#include "m_aux.h"
#include "m_gfx.h"
#include "m_snd.h"
#include "m_snd_data.h"
#include "m_data.h"
#include "m_gfx_data.h"
#include "m_scr.h"
#include "m_demo.h"

unsigned char ChangeScreen(int flag);
int IsHurt(int x, int y, int index, int index2);
unsigned char IsTouch(int x, int y, int index);
int UpdateFuel();
void DoShip();
void ReEnableBase();
void DoBase();
void UpdateScore(int update);
void BlowUpEnemy(int i);
int UpdateLaser(int i);
void DoLaser();
int UpdateAnimation(int f);
int UpdateMoveSpeed(int f);
void DoEnemy(int f);
void InitShip();
int GetFreeEnemyIndex();
void InitEnemies();
void InitNewScreen();
void InitNewGame();
int UpdateLives();
void RestartLevel();
void BlitStatus();
void DoTitle();
void DoWinScreen();
void DoGame();

unsigned char *pScreenBuffer = 0;
unsigned char ScreenTilesBuffer[0x2a8];
unsigned char ship_cur_screen = 0;

#define SH_DEAD 0
#define SH_ACTIVE 1

typedef struct
{
	int x;
	int y;
	int i;
	int state;
	int cur_frame;

	unsigned char anim_speed_cnt;
	unsigned char move_speed_cnt;
	unsigned char ai_update_cnt;
	// if these are chars, watcom generates erroneous sign extensions in (int) += (char)
	// because in watcom char's are unsigned by default, use -j switch
	int dx;
	int dy;

	unsigned char min_frame;
	unsigned char max_frame;
	unsigned char anim_speed;
	unsigned char move_speed;
	unsigned char ai_type;
	int parent;
} TSHIP;

#define SHIPS_NUMBER 13
TSHIP Ships[SHIPS_NUMBER];

unsigned char player_attached = 0;
int base_cur_screen;
int base_level_start = 1;
char screen_procedure;
int screen_bridge = 0;
int game_level = 1;
int ship_fuel = 5000;
int ship_lives = 10;
int ship_score = 0;
int laser_overload = 0;

#define GM_EXIT 	(-1)
#define GM_TITLE	0
#define GM_GAME		1
#define GM_CUT		2
#define GM_GAMEOVER	3
#define GM_YOUWIN	4
#define GM_DEMO		5
#define GM_PAUSE	6
#define GM_SPLASH	7

int game_mode = GM_SPLASH;
int elevator_flag = 0; // 1 if elevator is working

unsigned char GKeys[7]; // left, right, up, down, fire, pause, quit

#define F_UP 0
#define F_RIGHT 1
#define F_DOWN 2
#define F_LEFT 3

unsigned char ChangeScreen(int flag)
{
	unsigned char  result = *(SCREENINFOS[ship_cur_screen] + flag);

	if(result == 0) return 0;

	// if base is blocking the way on the other screen
	if(result == base_cur_screen) {
		if(flag == F_LEFT) {
			if(Ships[1].x >= 320 - 32 - 40 && Ships[0].y + 12 >= Ships[1].y) return 0;
		}
		if(flag == F_RIGHT) {
			if(Ships[1].x < 32 && Ships[0].y + 12 >= Ships[1].y) return 0;
		}
	}

	if(player_attached == 1) screen_bridge = 1; else screen_bridge = 0;

	ship_cur_screen = result;
	return 1;
}

int IsHurt(int x, int y, int index, int index2)
{
	int xs, ys, xs2, ys2;
	TSHIP *i = &Ships[index];
	TSHIP *j = &Ships[index2];

	if(index == 1 && ship_cur_screen != base_cur_screen) return 0;

	// sprite dimensions
	xs = **(pSprites256[i->i] + i->cur_frame);
	ys = *(*(pSprites256[i->i] + i->cur_frame) + 1);

	ys += y; xs += x;

	if(j->state != SH_DEAD) {
		// ignore bridge, explosions and elevator
		if(j->ai_type == 9 || j->ai_type == 8 || j->ai_type == 11) return 0;

		xs2 = j->x + **(pSprites256[j->i] + j->cur_frame);
		ys2 = j->y + *(*(pSprites256[j->i] + j->cur_frame) + 1);

		if(x < j->x) if(xs > j->x) goto _wdw2;
		if(x >= j->x) {
			if(xs2 > x) {
			_wdw2:
				if(y < j->y && ys > j->y) return 1;
				if(y >= j->y && ys2 > y) return 1;
			}
		}
	}

	return 0;
}

unsigned char IsTouch(int x, int y, int index)
{
	int xs, ys;
	TSHIP *i = &Ships[index];

	// sprite dimensions
	xs = **(pSprites256[i->i] + i->cur_frame);
	ys = *(*(pSprites256[i->i] + i->cur_frame) + 1);

	if(x < 0) return 1;
	if(y < 0) return 1;

	ys += y; xs += x;

	if(xs > 320) return 1;
	if(ys > 136) return 1;

	for(int dy = y; dy <= ys - 1; dy++) {
		for(int dx = x; dx <= xs - 1; dx++) {
			unsigned char b = ScreenTilesBuffer[(dy >> 3) * 40 + (dx >> 3)];

			if(index == 0) { // ship dies from touching certain tiles
				if(b > 0 && b < 246) return 1;
				if(b >= 246) BlowUpEnemy(index);
			} else {
				// for all others all tiles are solid
				if(b > 0) return 1;
			}
		}
	}

	for(int f = 0; f <= SHIPS_NUMBER - 1; f++) {
		if(&Ships[f] != i) { // don't compare with itself!
			if(Ships[f].state != SH_DEAD) {

				if(index > 1) {
					// ignore sparkles
					if(Ships[f].ai_type == 7) continue;
					if(Ships[f].ai_type == 3) continue;
					// homing missiles
					if(Ships[f].ai_type == 5) continue;
					// and bridge
					if(Ships[f].ai_type == 9) continue;
					// and bullets
					if(Ships[f].ai_type == 10) continue;

					if(Ships[index].ai_type == 3 || Ships[index].ai_type == 7) {
						if(f > 1) return 0;
					}

					if(IsHurt(x, y, index, f) == 1) {
						if(f < 2) BlowUpEnemy(f);
						return 1;
					}
					continue;
				} else {
					if(IsHurt(x, y, index, f) == 1) {
						if(f > 1) BlowUpEnemy(index);
						return 1;
					}
					continue;
				}
			}
		}
	}
	return 0;
}



int UpdateFuel()
{
	static int fuel_cnt = 25;

	if( fuel_cnt == 0) {
		if(ship_fuel == 0) {
			game_mode = GM_GAMEOVER;
			LM_ResetKeys();
			PutString(8*16, 8*10, "NO FUERZA");
			return 1;
		}
		ship_fuel -= 1;
		fuel_cnt = 25;
	} else {
		fuel_cnt -= 1;
	}
	return 0;
}

void DoShip()
{
	static unsigned char fallflag = 1;
	static int dy;

	TSHIP *i;

	i = &Ships[0];

	if(i->ai_type == 8) {
		DoEnemy(0);
		return;
	}

	// check if ships are attached
	if(Ships[1].state == SH_ACTIVE && Ships[1].x == i->x - 4 && Ships[1].y == i->y + 12 && player_attached == 0) {
		// if facing left or right
		if(i->cur_frame == 0 || i->cur_frame == 6) {
			player_attached = 1;
			screen_bridge = 1;
			LM_SND_rad_play_sndfx(rad_sndfx2, 1, SF_NOTE(1, 7));
		}
	}

	// exit if attach mode is ON
	if(player_attached == 1) return;

	if(UpdateFuel() == 1) return;

	if(GKeys[KEY_RIGHT] == 1) {
		if(i->cur_frame == 0) {
			if(IsTouch(i->x + 2, i->y, 0) == 0) {
				i->x += 2;
			} else {
				if(i->x == 288 && ChangeScreen(F_RIGHT) == 1) {
					i->x = 0;
					InitNewScreen();
				}
			}
		} else {
			if(i->anim_speed_cnt == 0) {
				i->cur_frame -= 1;
				i->anim_speed_cnt = i->anim_speed;
			} else {
				i->anim_speed_cnt -= 1;
			}
		}
	}

	if(GKeys[KEY_LEFT] == 1) {
		if(i->cur_frame == 6) {
			if(IsTouch(i->x - 2, i->y, 0) == 0) {
				i->x -= 2;
			} else {
				if(i->x == 0 && ChangeScreen(F_LEFT) == 1) {
					i->x = 288;
					InitNewScreen();
				}
			}
		} else {
			if(i->anim_speed_cnt == 0) {
				i->cur_frame += 1;
				i->anim_speed_cnt = i->anim_speed;
			} else {
				i->anim_speed_cnt -= 1;
			}
		}
	}

	if(GKeys[KEY_UP] == 1) {
		if((i->y & 1) != 0) dy = 1; else dy = 2;
		if(IsTouch(i->x, i->y - dy, 0) == 0) {
			i->y -= dy;
		} else {
			if(i->y == 0 && ChangeScreen(F_UP) == 1) {
				i->y = 124;
				InitNewScreen();
			}
		}
	} else {
		if(GKeys[KEY_DOWN] == 1) {
			if((i->y & 1) != 0) dy = 1; else dy = 2;
			if(IsTouch(i->x, i->y + dy, 0) == 0) {
				i->y += dy;
			} else {
				if(i->y == 124 && ChangeScreen(F_DOWN) == 1) {
					i->y = 0;
					InitNewScreen();
				}
			}
		} else {
			if(fallflag == 0) {
				if(IsTouch(i->x, i->y + 1, 0) == 0) i->y += 1;
				fallflag = 1;
				if(i->y == 124 && ChangeScreen(F_DOWN) == 1) {
					i->y = 0;
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
	if(base_cur_screen != ship_cur_screen) {
		Ships[1].state = SH_DEAD;
	} else {
		Ships[1].state = SH_ACTIVE;
	}
}

void DoBase()
{
	TSHIP *i, *j;

	i = &Ships[0]; // ship
	j = &Ships[1]; // base

	if(j->state == SH_DEAD) {
		if(j->ai_type == 8) { // if exploded previously, reenable base at level start
			base_cur_screen = base_level_start;
			j->x = 160;
			j->y = 104;
			j->i = 1;
			j->min_frame = 0;
			j->cur_frame = 0;
			j->max_frame = 1;
			j->anim_speed = 0;
			j->anim_speed_cnt = 0;
			j->ai_type = 0;
		}
		return;
	}

	// if exploding
	if(j->ai_type == 8) {
		DoEnemy(1);
		return;
	}

	// do smth if attach mode ON
	if(player_attached == 1 && i->ai_type != 8) {
		if(GKeys[KEY_RIGHT] == 1) {
			if(i->cur_frame == 0) {
				j->cur_frame ^= 1;

				if(IsTouch(i->x + 2, i->y, 0) + IsTouch(j->x + 2, j->y, 1) == 0) {
					if(ScreenTilesBuffer[((j->y + 16) >> 3) * 40 + ((j->x + 40) >> 3)] != 0) {
						i->x += 2;
						j->x += 2;
					}
				} else {
					if(j->x == 280 && ChangeScreen(F_RIGHT) == 1) {
						i->x = 4;
						j->x = 0;
						base_cur_screen = ship_cur_screen;
						InitNewScreen();
					}
				}
			} else {
				if(i->anim_speed_cnt == 0) {
					i->cur_frame -= 1;
					i->anim_speed_cnt = i->anim_speed;
				} else {
					i->anim_speed_cnt -= 1;
				}
			}
		}

		if(GKeys[KEY_LEFT] == 1) {
			if(i->cur_frame == 6) {
				j->cur_frame ^= 1;

				if(IsTouch(i->x - 2, i->y, 0) + IsTouch(j->x - 2, j->y, 1) == 0) {
					if(ScreenTilesBuffer[((j->y + 16) >> 3) * 40 + ((j->x - 2) >> 3)] != 0) {
						i->x -= 2;
						j->x -= 2;
					}
				} else {
					if(j->x == 0 && ChangeScreen(F_LEFT) == 1) {
						i->x = 284;
						j->x = 280;
						base_cur_screen = ship_cur_screen;
						InitNewScreen();
					}
				}
			} else {
				if(i->anim_speed_cnt == 0) {
					i->cur_frame += 1;
					i->anim_speed_cnt = i->anim_speed;
				} else {
					i->anim_speed_cnt -= 1;
				}
			}
		}


		if(GKeys[KEY_UP] == 1) {
			// if bound with base and standing on a bridge - don't allow to fly up
			for(int f = 0; f <= 3; f++) {
				if(ScreenTilesBuffer[((j->y + 16) >> 3) * 40 + ((j->x + 8) >> 3) + f] == 245) return;
			}

			// if standing on an elevator which is lifting up - don't allow to fly up
			if(elevator_flag == 1) return;

			player_attached = 0;
			i->y -= 2;
		}
	}
}

void UpdateScore(int update)
{
	static int ship_score_cnt = 0;

	if(ship_score == 0) ship_score_cnt = 0;

	ship_score_cnt += update;
	if(ship_score_cnt > 15000) {
		ship_lives += 1;
		ship_score_cnt /= 15000;
	}
	ship_score += update;
}

void BlowUpEnemy(int i)
{
	// if already exploding - exit
	if(Ships[i].ai_type == 8) return;

	// some corrections for homing missiles
	if(Ships[i].i == 40 || Ships[i].i == 41) return;

	// if non-killable enemy - exit
	if(Ships[i].i == 11 || Ships[i].i == 42) return;

	// update score, exclude self-exploding bullets
	if(Ships[i].ai_type != 10 && i != 0 && i != 1) UpdateScore(Ships[i].ai_type * 100 * (game_level & 7) + (RandomInt() & 127));

	// if blowing base - zero player_attached
	if(i == 1) player_attached = 0;

	// reactivate parent cannon
	if(Ships[i].i == 34) Ships[Ships[i].parent].dx = 0;

	// special procedures for breakable walls
	if(Ships[i].i == 6) {
		#if defined(__DINGUX__) || defined(__DINGOO__)
		extern int level_cache_fl;
		level_cache_fl = 1; // doing some magic...
		#endif
		if(Ships[i].cur_frame == 0) {
			Ships[i].x -= 8;
			ScreenTilesBuffer[(Ships[i].y >> 3) * 40 + (Ships[i].x >> 3)] = 0;
			ScreenTilesBuffer[((Ships[i].y >> 3) + 1) * 40 + (Ships[i].x >> 3)] = 0;
		} else {
			ScreenTilesBuffer[(Ships[i].y >> 3) * 40 + (Ships[i].x >> 3) + 1] = 0;
			ScreenTilesBuffer[((Ships[i].y >> 3) + 1) * 40 + (Ships[i].x >> 3) + 1] = 0;
		}
	}

	Ships[i].ai_type = 8; // explosion
	if(Ships[i].i == 6) {
		Ships[i].i = 7;
	} else {
		if(Ships[i].i == 1) {
			// explosion's sprite is smaller than base's thus a little hack
			Ships[i].i = 23;
			Ships[i].x += 4;
			Ships[i].y += 4;
		} else {
			if(Ships[i].i == 0)	Ships[i].i = 23; else Ships[i].i = 2;
		}
	}
	Ships[i].min_frame = 0;
	Ships[i].max_frame = 2;
	Ships[i].cur_frame = 0;
	Ships[i].anim_speed = 6;
	Ships[i].anim_speed_cnt = 6;

	LM_SND_rad_play_sndfx(rad_sndfx1, 0, SF_NOTE(2, 4));
}



int IsLaserHit2(int x_start, int x_end, int y)
{
	int xs2, ys2, return_value = 0;

	// swap if x_start > x_end
	if(x_start > x_end) { x_start ^= x_end; x_end ^= x_start; x_start ^= x_end; }

	if(x_start <= 0) return_value = 1;
	if(x_end >= 319) return_value =  1;

	if(ScreenTilesBuffer[(y >> 3) * 40 + (x_start >> 3)] > 0) return_value = 1;
	if(ScreenTilesBuffer[(y >> 3) * 40 + (x_end >> 3)] > 0) return_value = 1;

	for(int f = 1; f <= SHIPS_NUMBER-1; f++) {
		if(Ships[f].state != SH_DEAD) {
			// exclude electric sparkles
			if(Ships[f].ai_type == 7) continue;
			if(Ships[f].ai_type == 3) continue;
			//if(Ships[f].ai_type == 8) continue;
			if(Ships[f].ai_type == 9) continue;
			if(Ships[f].ai_type == 10) continue;
			if(Ships[f].ai_type == 11) continue;

			xs2 = Ships[f].x + **(pSprites256[Ships[f].i] + Ships[f].cur_frame);
			ys2 = Ships[f].y + *(*(pSprites256[Ships[f].i] + Ships[f].cur_frame) + 1);

			if(y >= Ships[f].y && y < ys2) {
				if(x_start < Ships[f].x && x_end >= xs2) {
					BlowUpEnemy(f);
					continue;
				}

				for(int dx = x_start; dx <= x_end; dx++) {
					if(dx >= Ships[f].x && dx < xs2) {
						BlowUpEnemy(f);
						return 1;
					}
				}
			}
		}
	}

	return return_value;
}

int UpdateLaser(int i)
{
	laser_overload += i;
	if(laser_overload > 32*8-1) {laser_overload = 0; return 1; }
	if(laser_overload < 0) laser_overload = 0;

	return 0;
}

int laser_dir = 0;
static int x_start, x_end, ly;

// ugly procedure which animates laser
void DoLaser()
{
	static int laser_phase = 0, dx;

	if(GKeys[KEY_FIRE] == 1) {
		if(UpdateLaser(1) == 1) {BlowUpEnemy(0); LM_ResetKeys(); return;}
	} else {
		UpdateLaser(-1);
	}

	// if zero - no shooting, 1 - shooting right, -1 - shooting left
	if(laser_dir == 0) {
		if(GKeys[KEY_FIRE] == 1 && elevator_flag == 0) { // HACK, or you will shoot your base
			//if ship facing right
			if(Ships[0].cur_frame == 0) {
				x_start = Ships[0].x + 32;
				x_end = x_start;
				ly = Ships[0].y + 6;
				laser_dir = 1;
				laser_phase = 0;
				goto __woo;
			} else { // if facing left
				if(Ships[0].cur_frame == 6) {
					x_start = Ships[0].x - 1;
					x_end = x_start;
					ly = Ships[0].y + 6;
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
		if(laser_dir == 1) {
			// another dirty hack or laser will overlap with ship when moving
			//x_start = Ships[0].x + 32;

			if(laser_phase == 0) {
				x_start = Ships[0].x + 32;

				for(dx = 0; dx <= 11; dx++) {
					x_end += 1;
					if(IsLaserHit2(x_start, x_end, ly) == 1) {
						laser_phase = 1;
						break;
					}
				}
			} else {
				for(dx = 0; dx <= 11; dx++) {
					x_start += 1;
					if(x_start == x_end) {
						laser_dir = 0;
						break;
					}
					IsLaserHit2(x_start, x_end, ly);
				}
			}
		} else { // shooting left
			// another dirty hack or laser will overlap with ship when moving
			//x_start = Ships[0].x - 1;

			if(laser_dir == -1) {
				if(laser_phase == 0) {
					x_start = Ships[0].x - 1;

					for(dx = 0; dx <= 11; dx++) {
						x_end -= 1;
						if(IsLaserHit2(x_start, x_end, ly) == 1) {
							laser_phase = 1;
							break;
						}
					}
				} else {
					for(dx = 0; dx <= 11; dx++) {
						x_start -= 1;
						if(x_start == x_end) {
							laser_dir = 0;
							break;
						}
						IsLaserHit2(x_start, x_end, ly);
					}
				}
			}
		}
	}
}

// update animation counters
// returns 1 if reached the end of animation cycle
int UpdateAnimation(int f)
{
	TSHIP *i;

	i = &Ships[f];

	// do animation counters
	if( i->anim_speed_cnt == 0) {
		i->anim_speed_cnt = i->anim_speed;
		i->cur_frame += 1;
		if(i->cur_frame > i->max_frame) {
			i->cur_frame = i->min_frame;
			return 1;
		}
	} else {
		i->anim_speed_cnt -= 1;
	}

	return 0;
}

// returns 1 if reached the end of move-wait cycle
int UpdateMoveSpeed(int f)
{
	TSHIP *i;

	i = &Ships[f];
	if(i->move_speed_cnt == 0) {
		i->move_speed_cnt = i->move_speed;
		return 1;
	} else {
		i->move_speed_cnt -= 1;
	}

	return 0;
}

void DoEnemy(int f)
{
	TSHIP *i;

	i = &Ships[f];

	if(i->state == SH_DEAD) return;

	// if main ship is exploding, freeze other enemies except bridge
	// it's not safe but ship dies after all and enemy data is reinitialized then
	if(f != 0 && Ships[0].ai_type == 8) if(i->ai_type != 9) i->ai_type = 0; // don't affect ship itself

	// do different ai types
	switch(i->ai_type) {
		case 0: // breakable wall or non-moving enemy
			UpdateAnimation(f);
			break;
		case 1: // randomly moving enemy
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1) {
				if(i->ai_update_cnt == 0) {
					i->dx = RandomInt() & 3;
					if(i->dx >= 2) i->dx = -1;

					i->dy = RandomInt() & 3;
					if(i->dy >= 2) i->dy = -1;

					i->ai_update_cnt = RandomInt() & 0x1f;
					if(i->ai_update_cnt < 15) i->ai_update_cnt = 15;
				} else {
					i->ai_update_cnt -= 1;
				}
			_optimize1:
				if(IsTouch(i->x + i->dx, i->y, f) == 0) i->x += i->dx; else i->ai_update_cnt = 0;
				if(IsTouch(i->x, i->y + i->dy, f) == 0) i->y += i->dy; else i->ai_update_cnt = 0;
			}
			break;
		case 2: // kamikadze
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1) {
				if(i->ai_update_cnt == 0) {
					if(i->x > Ships[0].x) {
						i->dx = -1;
					} else {
						if(i->x < Ships[0].x) i->dx = 1; else i->dx = 0;
					}

					if(i->y > Ships[0].y) {
						i->dy = -1;
					} else {
						if(i->y < Ships[0].y) i->dy = 1; else i->dy = 0;
					}

					i->ai_update_cnt = 15;
				} else {
					i->ai_update_cnt -= 1;
				}
				goto _optimize1;
			}
			break;

		case 3: // electric sparkle going up-down
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1) {
				if(i->dy == 0) i->dy = 1;
				if(IsTouch(i->x, i->y + i->dy, f) == 0) i->y += i->dy; else i->dy = -i->dy;
			}
			break;
		case 4: // ceiling cannon spawning kamikazes
			// if object is spawned - do nothig
			if(i->dx == 1) return;
			if(UpdateAnimation(f) == 1) {
				i->dx = 1;


				{ // spawn new enemy
					TSHIP *j;

					j = &Ships[0] + GetFreeEnemyIndex();

					j->state = SH_ACTIVE;
					j->i = 34;
					j->x = i->x;
					j->y = i->y + 16;
					j->anim_speed = 4;
					j->anim_speed_cnt = j->anim_speed;
					j->move_speed = 0;
					j->move_speed_cnt = j->move_speed;
					j->min_frame = 0;
					j->cur_frame = j->min_frame;
					j->max_frame = 3;
					j->ai_type = 2;
					j->parent = f;
				}
			}
			break;
		case 5: // homing missile
			UpdateAnimation(f);

			if(i->x > 0) {
				i->x -= 2;
				IsTouch(i->x, i->y, f);
				if(i->x < Ships[0].x) return;
				if(i->y > Ships[0].y) i->y -= 1;
				if(i->y < Ships[0].y && i->y < 63) {if((RandomInt() & 1) == 1) i->y += 1;}
			} else {
				i->x = 296;
				i->y = RandomInt() & 63;
				if(i->i == 40) i->i = 41; else i->i = 40;
			}
			break;
		case 6: // cannon
			if(i->x - 40 > Ships[0].x) {
				i->cur_frame = 0;
			} else {
				if(i->x + 40 < Ships[0].x) i->cur_frame = 2; else i->cur_frame = 1;
			}

			if((RandomInt() & 255) > 252) {
				TSHIP *j;

				j = &Ships[0] + GetFreeEnemyIndex();

				if(j->state == SH_DEAD) {
					j->state = SH_ACTIVE;
					j->i = 43;
					if(i->cur_frame == 0) {j->x = i->x - 4; j->y = i->y + 4; j->dx = -1; j->dy = -1;}
					if(i->cur_frame == 1) {j->x = i->x + 6; j->y = i->y - 4; j->dx = 0; j->dy = -1;}
					if(i->cur_frame == 2) {j->x = i->x + 16; j->y = i->y + 4; j->dx = 1; j->dy = -1;}
					j->anim_speed = 4;
					j->anim_speed_cnt = j->anim_speed;
					j->move_speed = 0;
					j->move_speed_cnt = j->move_speed;
					j->min_frame = 0;
					j->cur_frame = j->min_frame;
					j->max_frame = 0;
					j->ai_type = 10;
					j->parent = f;
				}
			}
			break;
		case 7: // electric sparkle going left-right
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1) {
				if(i->dx == 0) i->dx = 1;
				if(IsTouch(i->x + i->dx, i->y, f) == 0) i->x += i->dx; else i->dx = -i->dx;
			}
			break;
		case 8: // explosion, do one animation cycle and deactivate enemy entry
			if(UpdateAnimation(f) == 1) {
				i->state = SH_DEAD;
				if(f == 0) RestartLevel();
				return;
			}
			break;
		case 9: // bridge, appear if bonded ship, disappear otherwise
			{
				int a = 0;

				if(/*screen_bridge == 1 &&*/ player_attached == 1) a = 245;
				#if defined(__DINGUX__) || defined(__DINGOO__)
				{
					extern int level_cache_fl;
					level_cache_fl = 1;
				}
				#endif

				// seal or unseal the floor
				for(int f = 0; f <= 4; f++) {
					ScreenTilesBuffer[(i->y >> 3) * 40 + (i->x >> 3) + f] = a;
				}
			}
			break;
		case 10: // bullet

			// random exploding
			if(i->y + 16 < Ships[i->parent].y && (RandomInt() & 63) == 1) {BlowUpEnemy(f); return; }

			i->x += i->dx;
			if(i->x < 0 || i->x > 320) { BlowUpEnemy(f); return; }

			i->y += i->dy;
			if(i->y < 0) BlowUpEnemy(f);
			break;
		case 11: // elevator
			if(player_attached == 1) {
				// start to lift only when ship and base are standing on the elevator
				// ugly, improve in future
				//if((i->x == 256 && Ships[1].x >= 260) || (i->x == 16 && Ships[1].x <= 20))
				if(i->x == Ships[1].x - 4) {
					static int el_phase = 0;

					elevator_flag = 1;

					if(el_phase == 0) {
						// when starting to lift up - unseal the floor
						// ugly, maybe change in future
						if(i->y == 120) {
							#if defined(__DINGUX__) || defined(__DINGOO__)
							extern int level_cache_fl;
							level_cache_fl = 1; // doing some magic...
							#endif
							// unseal the floor
							for(int j = 0; j <= 5; j++) {
								ScreenTilesBuffer[(i->y >> 3) * 40 + (i->x >> 3) + j] = 0;
							}
						}

						// upper limit of the screen is reached
						if(Ships[0].y == 0) {
							el_phase = 1;
							Ships[0].y = 100;
							Ships[1].y = 112;

							ChangeScreen(F_UP);
							base_cur_screen = ship_cur_screen;
							InitNewScreen();

							// now i is invalid, because InitNewScreen reenables enemies
							// spawn new elevator
							i = &Ships[0] + GetFreeEnemyIndex();
							i->state = SH_ACTIVE;
							i->i = 21;
							i->x = Ships[1].x - 4;
							i->y = 128;
							i->ai_type = 11;
							goto _here;
						}
					} else {
						// if elevator is done lifting
						if(Ships[1].y == 104) {
							el_phase = 0;

							// seal the floor!
							for(int i = 0; i <= 5; i++) {
								#if defined(__DINGUX__) || defined(__DINGOO__)
								extern int level_cache_fl;
								level_cache_fl = 1; // doing some magic...
								#endif
								ScreenTilesBuffer[((Ships[1].y + 16) >> 3) * 40 + ((Ships[1].x - 4) >> 3) + i] = 245;
							}

							if(ship_cur_screen != 69) game_level += 1;
							base_level_start = ship_cur_screen;

							// destroy elevator or it will roll forever
							// but if not screen 69
							if(base_cur_screen != 69) {
								for(int j = 2; j <= SHIPS_NUMBER-2; j++) {
									if(Ships[j].ai_type == 11) Ships[j].state = SH_DEAD;
								}
							}

							elevator_flag = 0;
							goto _here;
						}
					}

					Ships[0].y -= 1;
					Ships[1].y -= 1;
					i->y -= 1;
				_here:;
				}
			}
			break;
	}
}

void InitShip()
{
	memset(&Ships[0] , 0, sizeof(TSHIP) * SHIPS_NUMBER);

	// flying ship data
	Ships[0].state = SH_ACTIVE;
	Ships[0].x = 152;
	Ships[0].y = 68;
	Ships[0].i = 0;
	Ships[0].min_frame = 0;
	Ships[0].cur_frame = ((game_level & 1) == 0 ? 6 : 0);
	Ships[0].max_frame = 6;
	Ships[0].anim_speed = 1;
	Ships[0].anim_speed_cnt = 1;

	// base data
	Ships[1].state = SH_ACTIVE;
	Ships[1].x = 148;
	Ships[1].y = 104;
	Ships[1].i = 1;
	Ships[1].min_frame = 0;
	Ships[1].cur_frame = 0;
	Ships[1].max_frame = 1;
	Ships[1].anim_speed = 0;
	Ships[1].anim_speed_cnt = 0;

}

int GetFreeEnemyIndex()
{
	for(int i = 2; i <= SHIPS_NUMBER-1; i++) {
		if(Ships[i].state == SH_DEAD) return i; // and ai_type should be zero!
	}

	return (SHIPS_NUMBER-1);
}

void InitEnemies()
{
	unsigned char *p;
	TSHIP *en;

	memset(&Ships[2] , 0, sizeof(TSHIP) * (SHIPS_NUMBER - 2));

	p = SCREENINFOS[ship_cur_screen] + 5;

	for(int i = *(p-1); i >= 1; i--) {
		if(*p != 128) { // <> 128 //64 is the bridge
			en = &Ships[0] + GetFreeEnemyIndex();
			en->state = SH_ACTIVE;
			en->i = *(p + 1);
			en->x = *(p + 2) << 2 ;
			en->y = *(p + 3);
			en->anim_speed = *(p + 4) * 2;
			en->anim_speed_cnt = en->anim_speed;
			en->min_frame = *(p + 5);
			en->cur_frame = en->min_frame;
			en->max_frame = *(p + 6);
			en->ai_type = *(p + 7);
			//if(*p == 64) en->ai_type = 9; // hack for bridge NOT NEEDED ANYMORE
			en->move_speed = 1; // standard
			en->move_speed_cnt = en->move_speed;
		}
		p += 8;
	}

	// generate enemies for elevator
	// 1 - right elevator, 2 - left elevator
	// sprite index - 21
	if(*p == 1 || *p == 2) {
		en = &Ships[0] + GetFreeEnemyIndex();
		en->state = SH_ACTIVE;
		en->i = 21;
		en->x = (*p == 1 ? 256 : 16);
		en->y = 120;
		en->anim_speed = 0;
		en->anim_speed_cnt = en->anim_speed;
		en->min_frame = 0;
		en->cur_frame = 0;
		en->max_frame = 0;
		en->ai_type = 11;
		en->move_speed = 1;
		en->move_speed_cnt = en->move_speed;
	}

	screen_procedure = *p;

}

void InitNewScreen()
{
	UnpackLevel();
	InitEnemies();
	ReEnableBase();
	laser_dir = 0;
}

void InitNewGame()
{
	ship_fuel = 5000;
	ship_lives = 10;
	ship_score = 0;

	ship_cur_screen = 1; // 70 - surface of the planet
	base_cur_screen = 1;
	base_level_start = 1;
	game_level = 1;

	player_attached = 0;

	InitShip();
	InitNewScreen();

	for(int f = 0; f <= 6; f++) {
		PutStream(0, STATUSBAR1[f][1] * 8, &STATUSBAR1[f][2]);
	}

}

int UpdateLives()
{
	ship_lives -= 1;
	if(ship_lives == 0) {
		game_mode = GM_GAMEOVER;
		LM_ResetKeys();
		PutString(8*16, 8*10, "HAS PERDIDO");
		return 1;
	}
	return 0;
}

void RestartLevel()
{
	if(UpdateLives() == 1) return;

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
	Int2ZString(game_level, 2, &string_buffer[0]);
	PutString(8*16, 8*20, &string_buffer[0]);

	// fuel
	Int2ZString(ship_fuel, 4, &string_buffer[0]);
	PutString(8*14, 8*21, &string_buffer[0]);

	// score
	Int2ZString(ship_score, 8, &string_buffer[0]);
	PutString(8*10, 8*22, &string_buffer[0]);

	// laser
	for(int i = 0; i <= 31; i++) {
		unsigned char c = ((i < (laser_overload >> 3)) ? 0x28 : 0);

		*(pScreenBuffer + i + 192 + 162 * 320) = c;
		*(pScreenBuffer + i + 192 + 162 * 320 + 320) = c;
		*(pScreenBuffer + i + 192 + 162 * 320 + 320 * 2) = c;
	}

	// lives
	Int2ZString(ship_lives, 2, &string_buffer[0]);
	PutString(8*28, 8*21, &string_buffer[0]);

	// score record
	PutString(8*22, 8*22, "88888888");
}


void RotateLogo()
{
	static float divisor, iterator;
	static int num_of_lines = 0;
	static int speed = 2;
	static int mirror = 0;
	static int sign = 1;

	if(speed > 0) {speed -= 1; return;}
	speed = 2;

	divisor = 24.0 / (24.0 - num_of_lines);
	iterator = 0.0;

	for(int i2 = num_of_lines; i2 <= 24; i2++) { // if put i2 <= 23, there's a blank line in the center of rotating sprite
		PutGeneric(96, (mirror == 0 ? 142 + i2 : 142 + 48 - i2), 33 * 4, 1, &LOGO[2 + 33 * (int)iterator]);
		PutGeneric(96, (mirror == 0 ? 142 + 48 - i2 : 142 + i2), 33 * 4, 1, &LOGO[2 + 47*33 - 33*(int)iterator]);

		iterator += divisor;
	}

	num_of_lines += sign;
	if(num_of_lines > 23) {mirror ^= 1; sign = -sign;}
	if(num_of_lines < 0) sign = -sign;
}

void DoSplash()
{
	static int ticks_for_splash = 0;

	if(ticks_for_splash == 0) {
		for(int i = 0; i <= 199; i++) {
			PutGeneric(0, i * 2, 320, 1, &splash_screen[i*80]);
			PutGeneric(0, i * 2 + 1, 320, 1, &splash_screen[i*80+8192]);
		}
	}

	ticks_for_splash += 1;
	if(ticks_for_splash > 350) game_mode = GM_TITLE;
	if(LM_AnyKey() == 1) {LM_ResetKeys(); game_mode = GM_TITLE;}
}

void DoTitle()
{
	static int _flag = 0;
	static int ticks_before_demo = 0;

	if(_flag == 0) {
		LM_SND_rad_play(rad_tune);
		memset(pScreenBuffer, 0, 320*200);
		ship_cur_screen = 0;
		_flag = 1;
		InitNewScreen();
		BlitLevel();
		PutSprite(50*4, 108, *(pSprites256[45] + 0));
		PutString(76, 88, "ESPACIO PARA COMENZAR");
	}

	RotateLogo();

	ticks_before_demo++;

	// wait some time before switching to demo mode
	if(ticks_before_demo >= 3660) { // wait 1 min
		ticks_before_demo = 0;
		ResetDemo();
		game_mode = GM_DEMO;
		_flag = 0;
		LM_SND_rad_stop();
		LM_ResetKeys();
		InitNewGame();
		return;
	}

	// exit to os
	if(Keys[SC_ESCAPE] == 1) {game_mode = GM_EXIT; return; }

	// start
	if(Keys[SC_SPACE] == 1 || Keys[SC_ENTER] == 1) {
		game_mode = GM_GAME;
		_flag = 0;
		LM_SND_rad_stop();
		LM_ResetKeys();
		InitNewGame();
	}
}

void DoWinScreen()
{
	static int flag = 0;
	static int i = 0;
	static char win_string[410] = ""
	"										"
	"ATENCION	ATENCION	 TRANSMISION A LA NAVE EXPLORER		   "
	"HAS CUMPLIDO TU ULTIMA MISION Y DEBES RETORNAR AL PLANETA NOVA DE LA GALAXIA TRAION"
	"										"
	"TU LUCHA NO HA SIDO EN VANO PUES LA LEJANA COLONIA DEL IMPERIO LLAMADA TIERRA HA SIDO "
	"LIBERADA DE LOS INVASORES Y PUEDE SER HABITADA DE NUEVO			   REPITO  MENSAJE";

	if(flag == 0) {
		memset(pScreenBuffer + 144 * 320, 0, 320 * 56);
		flag = 1;
		i = 0;
	}

	PutString(0 - i % 8, 20*8, &win_string[0] + i/8);

	if(i/8 >= sizeof(win_string)) i = 0; else i += 1;

	if(LM_AnyKey() == 1) {game_mode = GM_TITLE; flag = 0; LM_ResetKeys();}
}

void DoKeys()
{
	// if not demo mode
	if(game_mode != GM_DEMO) {
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
	if(laser_dir != 0) {
		if(laser_dir == 1) {
			for(int i = x_start; i <= x_end; i++)
				*(pScreenBuffer + ly * 320 + i) = 7;
		} else {
			for(int i = x_start; i >= x_end; i--)
				*(pScreenBuffer + ly * 320 + i) = 7;
		}
	}
}

void BlitEnemies()
{
	for(int i = 0; i <= SHIPS_NUMBER-1; i++) {
		if(Ships[i].state != SH_DEAD && Ships[i].ai_type != 9)
			PutSprite(Ships[i].x, Ships[i].y, *(pSprites256[Ships[i].i] + Ships[i].cur_frame));
	}
}

int frame_skip = 0;

void DoGame()
{
	switch(game_mode)
	{
		case GM_TITLE:
			// do title here
			DoTitle();
			break;
		case GM_DEMO:
			// demo mode here

			// if end playing demo
			if(PlayDemo() == 1 || LM_AnyKey() == 1) {
				game_mode = GM_TITLE;
				LM_ResetKeys();
				break;
			}
		case GM_GAME:
			DoKeys();

			if(GKeys[KEY_PAUSE] == 1) {
				PutString(8*17, 8*17, "PAUSA");
				game_mode = GM_PAUSE;
				Keys[SC_ENTER] = 0;
				break;
			}

			RecordDemo();

			if(Keys[SC_ESCAPE] == 1) {
				game_mode = GM_TITLE;
				LM_ResetKeys();
				break;
			}

			// win the game:
			if(screen_procedure == 3 /*&& base_cur_screen >= 70*/) {
				game_mode = GM_YOUWIN;
				LM_ResetKeys();
				LM_SND_rad_play(rad_tune);
				return;
			}

			// do enemies
			for(int i = 2; i <= SHIPS_NUMBER-1; i++) DoEnemy(i);

			DoShip();
			DoBase();
			DoLaser();

			if(frame_skip == 0) {
				BlitBackground(); // blit background
				// LATER: draw walls' outlines (shadows)
				// LATER: draw moving objects' outlines (shadows)
				BlitLevel(); // blit walls
				BlitEnemies(); // draw all enemies and cannon+base
				BlitLaser(); // don't forget laser
				BlitStatus(); // draw score etc
			}

			break;
		case GM_PAUSE:
				DoKeys();
				if(GKeys[KEY_PAUSE] == 1) {
					PutString(8*17, 8*17, "	 ");
					game_mode = GM_GAME;
					Keys[SC_ENTER] = 0;
				}
			break;
		case GM_GAMEOVER:
			if(LM_AnyKey() == 1) {InitNewGame(); LM_ResetKeys(); game_mode = GM_TITLE;}
			break;
		case GM_YOUWIN:
			DoWinScreen();
			break;
		case GM_SPLASH:
			DoSplash();
			break;
	}
}



int main(int argc, char *argv[])
{
	static char infostring[16] = "FPS: ";
	static int next_game_tick = 0;
	static int sleep_time = 0, frames = 0, frame_end = 0, frame_start = 0;
	static int show_fps = 0, max_frameskip = 0;

	if(LM_Init(&pScreenBuffer) == 0) return -1;
	LM_SND_rad_init();

	next_game_tick = LM_Timer();

	// main loop
	while(1)
	{
		//next_game_tick = LM_Timer();
		if(frames == 0) frame_start = next_game_tick;

		frame_skip = 0;

		LM_PollEvents();
		DoGame();
		if(show_fps == 1) PutString(8*0, 8*17, &infostring[0]);

		#ifdef __DOS__
		LM_GFX_WaitVSync();
		#endif

		// emulate slow cpu :) just for tests!
		//for(int i=0;i<12;i++)
		LM_GFX_Flip(pScreenBuffer);


		// in DOS fps shown will be incorrect a little
		// while real are always 75
		next_game_tick += 17; // gcc rounds (1000 / 60) to 16, but we need 17

		frames += 1;
		frame_end = LM_Timer();

		if(frame_end - frame_start >= 1000)
		{
			if(show_fps == 1) word2string(frames, &infostring[0] + 5);
			frames = 0;
		}

		#ifndef __DOS__
		sleep_time = next_game_tick - frame_end;
		if(sleep_time > 0)
		{
			LM_Sleep(sleep_time);
		} else { // slow computer, do frameskip
			while(sleep_time < 0 && frame_skip < max_frameskip) // max frame skip
			{
				sleep_time += 17; // 1000/60
				frame_skip += 1;
				LM_Sleep(2);
				LM_PollEvents();
				DoGame();
			}
			next_game_tick = LM_Timer();
		}
		#else
		next_game_tick = LM_Timer();
		#endif

		if(Keys[SC_BACKSPACE] == 1) {max_frameskip ^= 1; Keys[SC_BACKSPACE] = 0;}
		if(game_mode == GM_EXIT) break;
	}

	LM_SND_rad_deinit();
	LM_Deinit();

	return 0;
}
















