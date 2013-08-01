/*
	The Last Mission Remake

	Game engine, rewritten and reimagined :)
	Dmitry Smagin exmortis[REPLACE_WITH_AT_SYMBOL]yandex.ru

*/

#include <string.h>
#include <math.h>
#include "m_core.h"
#include "m_aux.h"
#include "m_gfx.h"
#include "m_data.h"
#include "m_gfx_data.h"
#include "m_scr.h"
#include "m_demo.h"

#include "sound.h"

#if DEBUG
#define M_INLINE
#else
#define M_INLINE inline
#endif

#define GAME_START_SCREEN 1 // Start of the labyrinth.

// Sheep is undestroyable.
// Used to test the complete app flow, win screen, etc.
//#define GOD_MODE

unsigned char ChangeScreen(int flag);
int IsHurt(int x, int y, int index, int index2);
unsigned char IsTouch(int x, int y, int index);
int UpdateFuel();
void DoShip();
void ReEnableBase();
void DoBase();
void BlowUpEnemy(int i);
int UpdateLaser(int i);
void DoLaser();
void DoMachineGun();
void DoBFG();
void DoRocketLauncher();
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
void CreateExplosion(int index);
void CreateWallExplosion(int x, int y);
void RenderGame(int renderStatus);
void InitGaragesForNewGame();
void SetGarageShipIndex(int garageId, int shipIndex);
int GarageShipIndex(int garageId);
int GetPlayerShipIndex();

// Game callbacks.
void HitTheBonus(int);
void PublishScore();
void GameLevelUp();

//#define MAKE_SCREENSHOTS
#ifdef MAKE_SCREENSHOTS
void MakeScreenShot();
#else
#define MakeScreenShot()
#endif

unsigned char *pScreenBuffer = 0;
unsigned char ScreenTilesBuffer[0x2a8];
unsigned char ship_cur_screen = 0;

typedef struct
{
	int regenerate_bonus;
	int bonus_type;
} TEXPLOSION;

typedef struct
{
	int x;
	int y;
	int i; // Sprite index
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

	// AI-type specific data. Just to make code a bit more readable.
	union
	{
		int parent; // used by something shot by a cannon
		int just_created; // used by AI_SHOT, AI_HOMING_SHOT and AI_BFG_SHOT
		int ticks_passed; // used by AI_HOMING_SHOT
		int garage_inactive; // used by AI_GARAGE
		int garage_index; // used by AI_SPARE_SHIP
		TEXPLOSION explosion; // used by AI_BONUS & AI_EXPLOSION
	};
} TSHIP;

#define SHIPS_NUMBER 32

TSHIP Ships[SHIPS_NUMBER];

// Actual garage data, valid for current game.
int garage_data[MAX_GARAGES][2];
// Garage data of the last game start. Will be restored if player is dead.
int main_garage_data[MAX_GARAGES][2];

typedef struct
{
	int xc, yc;
	int xt, yt;
	int ship;
	int ship_i;
	int hit_now;
	int hit_count;
} TBFGTARGET;

#define BFG_KILL_DISTANCE 60
#define BFG_KILL_TIME 16
#define MAX_BFG_TARGETS 13
TBFGTARGET BfgTargets[MAX_BFG_TARGETS];
int bfg_on = 0;

TSHIP *PrepareFreeShip();
void BestPositionInGarage(TSHIP *ship, int *x, int *y);

unsigned char player_attached = 0;
int base_cur_screen;
int base_level_start = 1;
char screen_procedure;
int screen_bridge = 0;
int game_level = 1;
int ship_fuel = 5000;
int ship_lives = 10;
int ship_health = 3;
int ticks_for_damage = 0;
int ship_score = 0;
int laser_overload = 0;
int ticks_for_splash = 0;
int easy_level = 1;
int sn_enabled = 1; // Facebook/Twitter integration.
int game_mode = GM_SPLASH;
int elevator_flag = 0; // 1 if elevator is working
int frame_skip = 0;
int modern_background = 1;
int title_start_flag = 0;
int youwin_start_flag = 0;
int cur_screen_bonus = 0;
int hidden_level_entered = 0;

unsigned char GKeys[7]; // left, right, up, down, fire, pause, quit

#define F_UP 0
#define F_RIGHT 1
#define F_DOWN 2
#define F_LEFT 3

void PlayMusicInt(int music)
{
	static int current_music = MUSIC_STOP;
	if (current_music != music)
	{
		current_music = music;
		PlayMusic(current_music);
	}
}

void SetGameMode(int mode)
{
	game_mode = mode;
	switch (mode)
	{
	case GM_TITLE:
		PlayMusicInt(MUSIC_INTRO);

		// To trigger enter actions.
		title_start_flag = 0;
		break;

	case GM_GAMEOVER:
		PlayMusicInt(MUSIC_LOSE);
		break;

	case GM_YOUWIN:
		PlayMusicInt(MUSIC_WIN);

		// To trigger enter actions.
		youwin_start_flag = 0;
		break;

	case GM_GAME:
	case GM_DEMO:
		PlayMusicInt(MUSIC_GAME);
		break;

	case GM_PAUSE:
		break;

	case GM_SPLASH:
		PlayMusicInt(MUSIC_STOP);
		break;
	}
}

unsigned char ChangeScreen(int flag)
{
	unsigned char  result = *(SCREENINFOS[ship_cur_screen] + flag);

	if(result == 0) return 0;

	// if base is blocking the way on the other screen
	if(result == base_cur_screen)
	{
		if(flag == F_LEFT)
		{
			if(Ships[1].x >= SCREEN_WIDTH - 32 - 40 && Ships[0].y + 12 >= Ships[1].y) return 0;
		}
		if(flag == F_RIGHT)
		{
			if(Ships[1].x < 32 && Ships[0].y + 12 >= Ships[1].y) return 0;
		}
	}

	if(player_attached == 1) screen_bridge = 1; else screen_bridge = 0;

	ship_cur_screen = result;
	return 1;
}

M_INLINE void GetCurrentSpriteDimensions(TSHIP *i, int *cx, int *cy)
{
	if (i->ai_type == AI_GARAGE)
	{
		*cx = GARAGE_WIDTH;
		*cy = GARAGE_HEIGHT;
	}
	else if (i->ai_type == AI_HIDDEN_AREA_ACCESS)
	{
		*cx = i->dx;
		*cy = i->dy;
	}
	else
	{
		*cx = **(pSprites256[i->i] + i->cur_frame);
		*cy = *(*(pSprites256[i->i] + i->cur_frame) + 1);
	}
}

M_INLINE int ShipBaseOffset()
{
	int xs, ys, xb, yb;
	GetCurrentSpriteDimensions(&Ships[0], &xs, &ys);
	GetCurrentSpriteDimensions(&Ships[1], &xb, &yb);
	return (xb - xs) / 2;
}

M_INLINE unsigned char TileForPoint(int x, int y)
{
	return ScreenTilesBuffer[(y >> 3) * 40 + (x >> 3)];
}

M_INLINE int IndexOf(TSHIP *ship)
{
	return ship - &Ships[0];
}

M_INLINE int FacingLeft(TSHIP *i)
{
	return i->cur_frame == i->max_frame;
}

M_INLINE int FacingRight(TSHIP *i)
{
	return i->cur_frame == i->min_frame;
}

M_INLINE int MaxRightPos(TSHIP *i)
{
	int x, y;
	GetCurrentSpriteDimensions(i, &x, &y);
	return SCREEN_WIDTH - x;
}

M_INLINE int MaxBottomPos(TSHIP *i)
{
	int x, y;
	GetCurrentSpriteDimensions(i, &x, &y);
	return ACTION_SCREEN_HEIGHT - y;
}

void CleanupBfg()
{
	memset(BfgTargets, 0, sizeof(BfgTargets));
	bfg_on = 0;
}

int IsOverlap(TSHIP *i, TSHIP *j)
{
	int xs, ys, xs2, ys2;

	GetCurrentSpriteDimensions(i, &xs, &ys);
	GetCurrentSpriteDimensions(j, &xs2, &ys2);

	ys += i->y;
	xs += i->x;

	ys2 += j->y;
	xs2 += j->x;

	// Retrn 1 if rectangles do intersect.
	if(i->x < j->x)
		if(xs > j->x)
			goto _wdw2;

	if(i->x >= j->x)
	{
		if(xs2 > i->x)
		{
		_wdw2:
			if ((i->y < j->y && ys > j->y) || (i->y >= j->y && ys2 > i->y))
				return 1;
		}
	}

	return 0;
}

int IsHurt(int x, int y, int index, int index2)
{
	int xs, ys, xs2, ys2;

	TSHIP *i = &Ships[index];
	TSHIP *j = &Ships[index2];

	if (index == 1 && ship_cur_screen != base_cur_screen)
		return 0;

	if (j->state == SH_DEAD)
		return 0;

	// ignore not-dangerous objects:
	// bridge, smoke, explosions and elevator
	if (j->ai_type == AI_BRIDGE ||
		j->ai_type == AI_EXPLOSION ||
		j->ai_type == AI_SMOKE ||
		j->ai_type == AI_ELEVATOR ||
		j->ai_type == AI_GARAGE)
		return 0;

	GetCurrentSpriteDimensions(i, &xs, &ys);
	GetCurrentSpriteDimensions(j, &xs2, &ys2);

	ys += y;
	xs += x;

	ys2 += j->y;
	xs2 += j->x;

	// Retrn 1 if rectangles do intersect.
	if(x < j->x)
		if(xs > j->x)
			goto _wdw2;

	if(x >= j->x)
	{
		if(xs2 > x)
		{
		_wdw2:
			if ((y < j->y && ys > j->y) || (y >= j->y && ys2 > y))
				return 1;
		}
	}

	return 0;
}

void HandleShipsContact(int i, int j)
{
	if (i < 2 && j < 2)
	{
		// Both objects are player controlled.
		return;
	}

	if (i > 1 && j > 1)
	{
		// Bot objects are NPC.
		if (Ships[i].ai_type != AI_SHOT &&
			Ships[j].ai_type != AI_SHOT &&
			Ships[i].ai_type != AI_HOMING_SHOT &&
			Ships[j].ai_type != AI_HOMING_SHOT &&
			Ships[i].ai_type != AI_BFG_SHOT &&
			Ships[j].ai_type != AI_BFG_SHOT)
		{
			return;
		}
	}

	if ((Ships[i].ai_type == AI_SPARE_SHIP && j < 2) ||
		(Ships[j].ai_type == AI_SPARE_SHIP && i < 2))
	{
		return;
	}

	if (Ships[i].ai_type == AI_BFG_SHOT)
	{
		BlowUpEnemy(j);
	}
	else if (Ships[j].ai_type == AI_BFG_SHOT)
	{
		BlowUpEnemy(i);
	}
	else if (Ships[i].ai_type == AI_BONUS)
	{
		if (j > 1)
		{
			// Bonus hit by a bullet. Swap bonus.
			Ships[i].explosion.regenerate_bonus = 1;
		}
		BlowUpEnemy(i);
	}
	else if (Ships[j].ai_type == AI_BONUS)
	{
		if (i > 1)
		{
			// Bonus hit by a bullet. Swap bonus.
			Ships[i].explosion.regenerate_bonus = 1;
		}
		BlowUpEnemy(j);
	}
	else
	{
		if (j > 1 || easy_level || i < 2)
			BlowUpEnemy(i);

		if (i > 1 || easy_level || j < 2)
			BlowUpEnemy(j);
	}
}

unsigned char IsTouch(int x, int y, int index)
{
	int xs, ys;
	TSHIP *i = &Ships[index];

	if (x < 0 || y < 0)
		return 1;

	GetCurrentSpriteDimensions(i, &xs, &ys);

	ys += y;
	xs += x;

	if (xs > SCREEN_WIDTH || ys > ACTION_SCREEN_HEIGHT)
		return 1;

	if (index <= 1)
	{
		// Player ship or base.
		for (int f = 0; f < SHIPS_NUMBER; f++)
		{
			if (f != index && Ships[f].state != SH_DEAD)
			{
				// don't compare with itself or with dead one!
				if (IsHurt(x, y, index, f))
				{
					HandleShipsContact(index, f);
					return 1;
				}
			}
		}
	}
	else
	{
		// Any other ship (non player-controlled).
		for (int f = 0; f < SHIPS_NUMBER; f++)
		{
			if (f == index || Ships[f].state == SH_DEAD)
			{
				// don't compare with itself or with dead one!
				continue;
			}

			// ignore sparkles
			if (Ships[f].ai_type == AI_ELECTRIC_SPARKLE_HORIZONTAL ||
				Ships[f].ai_type == AI_ELECTRIC_SPARKLE_VERTICAL ||
				Ships[f].ai_type == AI_HOMING_MISSLE ||
				Ships[f].ai_type == AI_BRIDGE ||
				Ships[f].ai_type == AI_BULLET)
				continue;

			if (Ships[index].ai_type == AI_ELECTRIC_SPARKLE_HORIZONTAL ||
				Ships[index].ai_type == AI_ELECTRIC_SPARKLE_VERTICAL)
			{
				if (f > 1)
					break;
			}

			if (IsHurt(x, y, index, f))
			{
				HandleShipsContact(f, index);

				if (Ships[index].ai_type == AI_BFG_SHOT)
					return 0;

				return 1;
			}
		}
	}

	for (int dy = y; dy < ys; dy++)
	{
		for (int dx = x; dx < xs; dx++)
		{
			unsigned char b = TileForPoint(dx, dy);
			if (b == 0)
				continue;

			// All tiles are solid.
			if (index == 0 && b >= 246)
			{
				// ship dies from touching certain tiles
				BlowUpEnemy(index);
			}

			return 1;
		}
	}

	return 0;
}

int UpdateFuel()
{
	static int fuel_cnt = 25;

	if( fuel_cnt == 0)
	{
		if(ship_fuel == 0)
		{
			SetGameMode(GM_GAMEOVER);
			LM_ResetKeys();
			PutString(8*16, 8*10, "NO FUERZA");
			return 1;
		}
		ship_fuel -= 1;
		fuel_cnt = 25;
	}
	else
	{
		fuel_cnt -= 1;
	}
	return 0;
}

void DoShip()
{
	static unsigned char fallflag = 1;
	static int dy;

	if (--ticks_for_damage < 0) ticks_for_damage = 0;

	TSHIP *i;

	i = &Ships[0];

	if(i->ai_type == AI_EXPLOSION)
	{
		DoEnemy(0);
		return;
	}

	// check if ships are attached
	if(Ships[1].state == SH_ACTIVE &&
	   Ships[1].ai_type != AI_EXPLOSION &&
	   player_attached == 0)
	{
		if(FacingLeft(i) || FacingRight(i))
		{
			int xs, ys, xb, yb;
			GetCurrentSpriteDimensions(i, &xs, &ys);
			GetCurrentSpriteDimensions(&Ships[1], &xb, &yb);
			if ((i->x + xs / 2 == Ships[1].x + xb / 2) &&
				(i->y + ys == Ships[1].y))
			{
				player_attached = 1;
				screen_bridge = 1;
				PlaySoundEffect(SND_CONTACT);
			}
		}
	}

	// exit if attach mode is ON
	if (player_attached == 1 ||
		UpdateFuel() == 1 ||
		Ships[0].state == SH_DEAD)
	{
		return;
	}

	if(GKeys[KEY_RIGHT] == 1)
	{
		if(FacingRight(i))
		{
			if(IsTouch(i->x + 2, i->y, 0) == 0)
			{
				i->x += 2;
			}
			else
			{
				if(i->x == MaxRightPos(i) && ChangeScreen(F_RIGHT) == 1)
				{
					i->x = 0;
					InitNewScreen();
				}
			}
		}
		else
		{
			if(i->anim_speed_cnt == 0)
			{
				i->cur_frame -= 1;
				i->anim_speed_cnt = i->anim_speed;
			}
			else
			{
				i->anim_speed_cnt -= 1;
			}
		}
	}

	if(GKeys[KEY_LEFT] == 1)
	{
		if(FacingLeft(i))
		{
			if(IsTouch(i->x - 2, i->y, 0) == 0)
			{
				i->x -= 2;
			}
			else
			{
				if(i->x == 0 && ChangeScreen(F_LEFT) == 1)
				{
					i->x = MaxRightPos(i);
					InitNewScreen();
				}
			}

		}
		else
		{
			if(i->anim_speed_cnt == 0)
			{
				i->cur_frame += 1;
				i->anim_speed_cnt = i->anim_speed;
			}
			else
			{
				i->anim_speed_cnt -= 1;
			}
		}
	}

	if(GKeys[KEY_UP] == 1)
	{
		if((i->y & 1) != 0) dy = 1; else dy = 2;
		if(IsTouch(i->x, i->y - dy, 0) == 0)
		{
			i->y -= dy;
		}
		else
		{
			if(i->y == 0 && ChangeScreen(F_UP) == 1)
			{
				i->y = MaxBottomPos(i);
				InitNewScreen();
			}
		}

	}
	else
	{
		if(GKeys[KEY_DOWN] == 1)
		{
			if((i->y & 1) != 0) dy = 1; else dy = 2;
			if(IsTouch(i->x, i->y + dy, 0) == 0)
			{
				i->y += dy;
			}
			else
			{
				if(i->y == MaxBottomPos(i) && ChangeScreen(F_DOWN) == 1)
				{
					i->y = 0;
					InitNewScreen();
				}
			}
		}
		else
		{
			if(fallflag == 0)
			{
				if(IsTouch(i->x, i->y + 1, 0) == 0) i->y += 1;
				fallflag = 1;
				if(i->y == MaxBottomPos(i) && ChangeScreen(F_DOWN) == 1)
				{
					i->y = 0;
					InitNewScreen();
				}
			}
			else
			{
				fallflag -= 1;
			}
		}
	}

}

void ReEnableBase()
{
	if(base_cur_screen != ship_cur_screen)
	{
		Ships[1].state = SH_DEAD;
	}
	else
	{
		Ships[1].state = SH_ACTIVE;
	}
}

void DoBase()
{
	TSHIP *i, *j;

	i = &Ships[0]; // ship
	j = &Ships[1]; // base

	if(j->state == SH_DEAD)
	{
		if(j->ai_type == AI_EXPLOSION) // if exploded previously, reenable base at level start
		{
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
	if(j->ai_type == AI_EXPLOSION)
	{
		DoEnemy(1);
		return;
	}

	// do smth if attach mode ON
	int playMoveSound = 0;

	if(player_attached == 1 && i->ai_type != AI_EXPLOSION)
	{
		if(GKeys[KEY_RIGHT] == 1)
		{
			if(FacingRight(i))
			{
				j->cur_frame ^= 1;
				playMoveSound = 1;

				if(IsTouch(i->x + 2, i->y, 0) + IsTouch(j->x + 2, j->y, 1) == 0)
				{
					if (TileForPoint(j->x + 40, j->y + 16))
					{
						i->x += 2;
						j->x += 2;
					}
				}
				else
				{
					if(j->x == 280 && ChangeScreen(F_RIGHT) == 1)
					{
						i->x = ShipBaseOffset();
						j->x = 0;
						base_cur_screen = ship_cur_screen;
						InitNewScreen();
					}
				}
			}
			else
			{
				if(i->anim_speed_cnt == 0)
				{
					i->cur_frame -= 1;
					i->anim_speed_cnt = i->anim_speed;
				}
				else
				{
					i->anim_speed_cnt -= 1;
				}
			}
		}

		if(GKeys[KEY_LEFT] == 1)
		{
			if(FacingLeft(i))
			{
				j->cur_frame ^= 1;
				playMoveSound = 1;

				if(IsTouch(i->x - 2, i->y, 0) + IsTouch(j->x - 2, j->y, 1) == 0)
				{
					if (TileForPoint(j->x - 2, j->y + 16))
					{
						i->x -= 2;
						j->x -= 2;
					}
				}
				else
				{
					if(j->x == 0 && ChangeScreen(F_LEFT) == 1)
					{
						//xxx
						i->x = 280 + ShipBaseOffset();
						j->x = 280;
						base_cur_screen = ship_cur_screen;
						InitNewScreen();
					}
				}
			}
			else
			{
				if(i->anim_speed_cnt == 0)
				{
					i->cur_frame += 1;
					i->anim_speed_cnt = i->anim_speed;
				}
				else
				{
					i->anim_speed_cnt -= 1;
				}
			}
		}

		if (playMoveSound) PlaySoundEffect(SND_MOVE);
		else StopSoundEffect(SND_MOVE);

		if(GKeys[KEY_UP] == 1)
		{
			// if bound with base and standing on a bridge - don't allow to fly up
			for(int f = 0; f <= 3; f++)
			{
				if(ScreenTilesBuffer[((j->y + 16) >> 3) * 40 + ((j->x + 8) >> 3) + f] == 245) return;
			}

			// if standing on an elevator which is lifting up - don't allow to fly up
			if(elevator_flag == 1) return;

			StopSoundEffect(SND_MOVE);

			player_attached = 0;
			i->y -= 2;
		}
	}
}

void AddScore(int update)
{
	const int points_per_life = 15000;

	int livesBefore = ship_score / points_per_life;

	ship_score += update;

	int livesAfter = ship_score / points_per_life;

	ship_lives += (livesAfter - livesBefore);
}

void UpdateScoreWithShip(int i)
{
	if (i > 1 &&
		Ships[i].ai_type != AI_BULLET &&
		Ships[i].ai_type != AI_SHOT &&
		Ships[i].ai_type != AI_BFG_SHOT &&
		Ships[i].ai_type != AI_HOMING_SHOT &&
		Ships[i].ai_type != AI_BONUS &&
		Ships[i].ai_type != AI_ELECTRIC_SPARKLE_VERTICAL &&
		Ships[i].ai_type != AI_ELECTRIC_SPARKLE_HORIZONTAL)
	{
		// /2 less points for hard mode, /2 less for rocket launcher of BFG
		int score = Ships[i].ai_type * 100 * (game_level & 7) + (RandomInt() & 127);
		if (easy_level) score >>= 1;
		if (Ships[0].i == SHIP_TYPE_ROCKET_LAUNCHER || Ships[0].i == SHIP_TYPE_BFG) score >>= 1;

		AddScore(score);
	}
}

void DestroyHiddenAreaAccess(TSHIP *i, int playEffects)
{
	// Cleanup background and make noise if needed.
	for (int y = 0; y < i->dy; ++y)
	{
		int ay = i->y + y;
		for (int x = 0; x < i->dx; ++x)
		{
			int ax = i->x + x;

			int index = (ay >> 3) * 40 + (ax >> 3);
			if (index >= 0x2a8)
				break;

			ScreenTilesBuffer[index] = 0;

			if (playEffects && !(y % 16) && !(x % 16) && (ay + 16 < ACTION_SCREEN_HEIGHT))
			{
				CreateWallExplosion(ax, ay);
			}
		}
	}

	if (playEffects)
	{
		PlaySoundEffect(SND_EXPLODE);
	}

	i->state = SH_DEAD;
}

void BlowUpEnemy(int i)
{
#ifdef GOD_MODE
	if (i <= 1)
	{
		// You cannot hurt a god.
		return;
	}
#endif

	if (i == 0)
	{
		// This is the player ship.
		if (easy_level)
		{
			if (!ticks_for_damage)
			{
				ticks_for_damage = 20;
				--ship_health;
			}
		}
		else
		{
			ship_health = -1;
		}

		if (ship_health >= 0)
		{
			return;
		}
	}

	if (i == 1)
	{
		// This is a base, do not kill the base while moving up,
		// otherwise player will be disapointed + there is a bug whith
		// elevator_flag being not reset.
		if (elevator_flag)
			return;
	}

	// if already exploding - exit
	if(Ships[i].ai_type == AI_EXPLOSION) return;
	if(Ships[i].ai_type == AI_SMOKE) return;

	if (Ships[i].ai_type == AI_SHOT ||
		Ships[i].ai_type == AI_HOMING_SHOT)
	{
		Ships[i].state = SH_DEAD;
		return;
	}

	if (Ships[i].ai_type == AI_BFG_SHOT)
	{
		CleanupBfg();
		Ships[i].state = SH_DEAD;
		return;
	}

	if (Ships[i].ai_type == AI_HIDDEN_AREA_ACCESS)
	{
		DestroyHiddenAreaAccess(Ships + i, 1);
		return;
	}

	// some corrections for homing missiles
	if(Ships[i].i == 40 || Ships[i].i == 41) {
		if (easy_level) {
			CreateExplosion(i);
			Ships[i].x += SCREEN_WIDTH;
			PlaySoundEffect(SND_EXPLODE);
		}
		return;
	}


	// if non-killable enemy - exit
	if(Ships[i].i == 11 || Ships[i].i == 42) return;

	MakeScreenShot();

	// update score with the killed ship data.
	UpdateScoreWithShip(i);

	// if blowing base - zero player_attached
	if(i == 1) player_attached = 0;

	// reactivate parent cannon
	if(Ships[i].i == 34) Ships[Ships[i].parent].dx = 0;

	// special procedures for breakable walls
	if(Ships[i].i == 6)
	{
		#if 0 //defined(__DINGUX__) || defined(__DINGOO__)
		extern int level_cache_fl;
		level_cache_fl = 1; // doing some magic...
		#endif
		if(Ships[i].cur_frame == 0)
		{
			Ships[i].x -= 8;
			ScreenTilesBuffer[(Ships[i].y >> 3) * 40 + (Ships[i].x >> 3)] = 0;
			ScreenTilesBuffer[((Ships[i].y >> 3) + 1) * 40 + (Ships[i].x >> 3)] = 0;
		}
		else
		{
			ScreenTilesBuffer[(Ships[i].y >> 3) * 40 + (Ships[i].x >> 3) + 1] = 0;
			ScreenTilesBuffer[((Ships[i].y >> 3) + 1) * 40 + (Ships[i].x >> 3) + 1] = 0;
		}
	}

	if (Ships[i].ai_type == AI_BONUS)
	{
		// Memorize bonus type.
		Ships[i].explosion.bonus_type = Ships[i].i;
	}

	if (easy_level &&
		(Ships[i].ai_type == AI_RANDOM_MOVE || Ships[i].ai_type == AI_KAMIKADZE))
	{
		// Generate bonus if defined by screen and if
		// this ship is the last one on the screen.
		if (cur_screen_bonus)
		{
			int alive_ship = 0;
			for (int n = 2; n < SHIPS_NUMBER; ++n)
			{
				if (n != i &&
					Ships[n].state == SH_ACTIVE &&
					(Ships[n].ai_type == AI_RANDOM_MOVE || Ships[n].ai_type == AI_KAMIKADZE))
				{
					alive_ship = 1;
					break;
				}
			}

			if (!alive_ship)
			{
				Ships[i].explosion.bonus_type = cur_screen_bonus;
				Ships[i].explosion.regenerate_bonus = 1;
			}
		}
	}

	Ships[i].ai_type = AI_EXPLOSION;
	if(Ships[i].i == 6)
	{
		Ships[i].i = 7;
	}
	else
	{
		if(Ships[i].i == 1)
		{
			// explosion's sprite is smaller than base's thus a little hack
			Ships[i].i = 23;
			Ships[i].x += 4;
			Ships[i].y += 4;
		}
		else
		{
			if(Ships[i].i == 0)	Ships[i].i = 23; else Ships[i].i = 2;
		}
	}
	Ships[i].min_frame = 0;
	Ships[i].max_frame = 2;
	Ships[i].cur_frame = 0;
	Ships[i].anim_speed = 6;
	Ships[i].anim_speed_cnt = 6;

	PlaySoundEffect(SND_EXPLODE);
}

int IsLaserHit2(int x_start, int x_end, int y)
{
	int xs2, ys2, return_value = 0;

	// swap if x_start > x_end
	if(x_start > x_end) { x_start ^= x_end; x_end ^= x_start; x_start ^= x_end; }

	if(x_start <= 0) return_value = 1;
	if(x_end >= 319) return_value =  1;

	if (TileForPoint(x_start, y)) return_value = 1;
	if (TileForPoint(x_end, y)) return_value = 1;

	for(int f = 1; f <= SHIPS_NUMBER-1; f++)
	{
		if(Ships[f].state != SH_DEAD)
		{
			// exclude non-hittable objects
			if (Ships[f].ai_type == AI_ELECTRIC_SPARKLE_HORIZONTAL ||
				Ships[f].ai_type == AI_ELECTRIC_SPARKLE_VERTICAL ||
				Ships[f].ai_type == AI_BRIDGE ||
				Ships[f].ai_type == AI_BULLET ||
				Ships[f].ai_type == AI_SHOT ||
				Ships[f].ai_type == AI_BFG_SHOT ||
				Ships[f].ai_type == AI_HOMING_SHOT ||
				Ships[f].ai_type == AI_GARAGE ||
				Ships[f].ai_type == AI_ELEVATOR)
			{
				continue;
			}

			int cx, cy;
			GetCurrentSpriteDimensions(Ships + f, &cx, &cy);

			xs2 = Ships[f].x + cx;
			ys2 = Ships[f].y + cy;

			if(y >= Ships[f].y && y < ys2)
			{
				if(x_start < Ships[f].x && x_end >= xs2)
				{
					if (Ships[f].ai_type == AI_BONUS) {
						Ships[f].explosion.regenerate_bonus = 1;
					}

					BlowUpEnemy(f);
					continue;
				}

				for(int dx = x_start; dx <= x_end; dx++)
				{
					if(dx >= Ships[f].x && dx < xs2)
					{
						if (Ships[f].ai_type == AI_BONUS) {
							Ships[f].explosion.regenerate_bonus = 1;
						}

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

void DoMachineGun()
{
	static int mg_timeout = 0;
	if (--mg_timeout < 0)
		mg_timeout = 0;

	if(GKeys[KEY_FIRE] == 1)
	{
		if(UpdateLaser(1) == 1)
		{
			BlowUpEnemy(0);
			LM_ResetKeys();
			return;
		}

		if (!mg_timeout)
		{
			// Create a new bullet.
			TSHIP *i = &Ships[0];

			//if ship is not facing right or left, then exit.
			if(!FacingLeft(i) && !FacingRight(i))
				return;

			TSHIP *j = PrepareFreeShip();
			j->state = SH_ACTIVE;
			j->i = 50;
			j->x = i->x + (FacingRight(i) ? 32 : -8);
			j->y = i->y + 5;
			j->dy = 0;
			j->dx = FacingRight(i) ? 5 : -5;
			j->anim_speed = 4;
			j->anim_speed_cnt = j->anim_speed;
			j->move_speed_cnt = j->move_speed;
			j->cur_frame = FacingRight(i) ? 0 : 1;
			j->ai_type = AI_SHOT;
			j->just_created = 1;

			// Reset timeout.
			mg_timeout = 20;

			PlaySoundEffect(SND_SHORT_LASER_SHOOT);
		}
	}
	else
	{
		UpdateLaser(-1);
		mg_timeout = 0;
	}
}

void DoBFG()
{
	static int mg_timeout = 0;
	if (--mg_timeout < 0)
		mg_timeout = 0;

	if(GKeys[KEY_FIRE] == 1)
	{
		if (!mg_timeout && !bfg_on)
		{
			// Create a new bullet.
			TSHIP *i = &Ships[0];

			//if ship is not facing right or left, then exit.
			if(!FacingLeft(i) && !FacingRight(i))
				return;

			TSHIP *j = PrepareFreeShip();
			j->state = SH_ACTIVE;
			j->i = 56;
			j->x = i->x + (FacingRight(i) ? 16 : -11);
			j->y = i->y + 5;
			j->dy = 0;
			j->dx = FacingRight(i) ? 2 : -2;
			j->anim_speed = 10;
			j->anim_speed_cnt = j->anim_speed;
			j->move_speed_cnt = j->move_speed;
			j->cur_frame = 0;
			j->max_frame = 3;
			j->ai_type = AI_BFG_SHOT;
			j->just_created = 1;

			bfg_on = 1;

			// Reset timeout.
			mg_timeout = 20;

			PlaySoundEffect(SND_CANNON_SHOOT);
		}
	}
	else
	{
		mg_timeout = 0;
	}
}

void DoRocketLauncher()
{
	static int rl_timeout = 0;
	if (--rl_timeout < 0)
		rl_timeout = 0;

	if(GKeys[KEY_FIRE] == 1)
	{
		if (!rl_timeout)
		{
			// Create a new bullet.
			TSHIP *i = &Ships[0];

			//if ship is not facing right or left, then exit.
			if(!FacingLeft(i) && !FacingRight(i))
				return;

			TSHIP *j = PrepareFreeShip();
			j->state = SH_ACTIVE;
			j->i = 54;
			j->y = i->y + 1;
			j->dy = 0;
			j->anim_speed = 4;
			j->anim_speed_cnt = j->anim_speed;
			j->move_speed_cnt = j->move_speed;
			j->ai_type = AI_HOMING_SHOT;
			j->just_created = 1;

			if (FacingRight(i))
			{
				// to the right
				j->x = i->x + 32;
				j->dx = 3 ;
				j->cur_frame = 2;
				j->min_frame = 2;
				j->max_frame = 3;
			}
			else
			{
				// to the left
				j->x = i->x - 14;
				j->dx = -3;
				j->cur_frame = 0;
				j->max_frame = 1;
			}

			PlaySoundEffect(SND_ROCKET_SHOOT);

			// Reset timeout.
			rl_timeout = 30;
		}
	}
	else
	{
		//rl_timeout --;
	}
}


int laser_dir = 0;
static int x_start, x_end, ly;

// ugly procedure which animates laser
void DoLaser()
{
	static int laser_phase = 0, dx;
	static int previous_phase = 0;

	int fireOn = (GKeys[KEY_FIRE] == 1) && (Ships[0].i == SHIP_TYPE_LASER);
	if (fireOn)
	{
		if(UpdateLaser(1) == 1) {BlowUpEnemy(0); LM_ResetKeys(); return;}
	}
	else
	{
		UpdateLaser(-1);
	}

	// if zero - no shooting, 1 - shooting right, -1 - shooting left
	if(laser_dir == 0)
	{
		if(fireOn && elevator_flag == 0) // HACK, or you will shoot your base
		{
			//if ship facing right
			if(FacingRight(&Ships[0]))
			{
				x_start = Ships[0].x + 32;
				x_end = x_start;
				ly = Ships[0].y + 6;
				laser_dir = 1;
				laser_phase = 0;
				goto __woo;
			}
			// if facing left
			else
			{
				if(FacingLeft(&Ships[0]))
				{
					x_start = Ships[0].x - 1;
					x_end = x_start;
					ly = Ships[0].y + 6;
					laser_dir = -1;
					laser_phase = 0;
					goto __woo;
				}
			}
		}
	}
	else
	{
		__woo:
		// animate laser
		// shooting right
		if(laser_dir == 1)
		{
			// another dirty hack or laser will overlap with ship when moving
			//x_start = Ships[0].x + 32;

			if(laser_phase == 0)
			{
				x_start = Ships[0].x + 32;

				for(dx = 0; dx <= 11; dx++)
				{
					x_end += 1;
					if(IsLaserHit2(x_start, x_end, ly) == 1)
					{
						laser_phase = 1;
						break;
					}
				}
			}
			else
			{
				for(dx = 0; dx <= 11; dx++)
				{
					x_start += 1;
					if(x_start == x_end)
					{
						laser_dir = 0;
						break;
					}
					IsLaserHit2(x_start, x_end, ly);
				}
			}

		}
		// shooting left
		else
		{
			// another dirty hack or laser will overlap with ship when moving
			//x_start = Ships[0].x - 1;

			if(laser_dir == -1)
			{
				if(laser_phase == 0)
				{
					x_start = Ships[0].x - 1;

					for(dx = 0; dx <= 11; dx++)
					{
						x_end -= 1;
						if(IsLaserHit2(x_start, x_end, ly) == 1)
						{
							laser_phase = 1;
							break;
						}
					}
				}
				else
				{
					for(dx = 0; dx <= 11; dx++)
					{
						x_start -= 1;
						if(x_start == x_end)
						{
							laser_dir = 0;
							break;
						}
						IsLaserHit2(x_start, x_end, ly);
					}
				}
			}

		}
	}

	if (fireOn)
	{
		if (laser_phase != previous_phase && !laser_phase) {
			PlaySoundEffect(SND_LASER_SHOOT);
		}

		previous_phase = laser_phase;
	}
	else
	{
		previous_phase = 1;
	}
}

// update animation counters
// returns 1 if reached the end of animation cycle
int UpdateAnimation(int f)
{
	TSHIP *i;

	i = &Ships[f];

	// do animation counters
	if( i->anim_speed_cnt == 0)
	{
		i->anim_speed_cnt = i->anim_speed;
		i->cur_frame += 1;
		if(i->cur_frame > i->max_frame)
		{
			i->cur_frame = i->min_frame;
			return 1;
		}
	}
	else
	{
		i->anim_speed_cnt -= 1;
	}

	return 0;
}

// returns 1 if reached the end of move-wait cycle
int UpdateMoveSpeed(int f)
{
	TSHIP *i;

	i = &Ships[f];
	if(i->move_speed_cnt == 0)
	{
		i->move_speed_cnt = i->move_speed;
		return 1;
	}
	else
	{
		i->move_speed_cnt -= 1;
	}

	return 0;
}

void CreateExplosion(int index)
{
	TSHIP *i = &Ships[index];
	TSHIP *j = PrepareFreeShip();
	j->state = SH_ACTIVE;
	j->i = 2;
	j->x = i->x;
	j->y = i->y;
	j->anim_speed = 6;
	j->anim_speed_cnt = 6;
	j->max_frame = 2;
	j->ai_type = AI_EXPLOSION;
}

int ShipsDistance(TSHIP *i, TSHIP *j)
{
	int x = i->x - j->x;
	int y = i->y - j->y;
	return (int)sqrt((float)(x*x + y*y));
}

void AddBfgTarget(int index, int bfgIndex)
{
	TSHIP *i = Ships + index;
	TSHIP *bfg = Ships + bfgIndex;

	TBFGTARGET *t = NULL;
	for (int n = 0; n < MAX_BFG_TARGETS; ++n)
	{
		if (BfgTargets[n].ship == index &&
			BfgTargets[n].ship_i == i->i)
		{

			t = &BfgTargets[n];
			break;
		}
		else if (!t && !BfgTargets[n].ship)
		{
			t = &BfgTargets[n];
		}
	}

	if (!t)
		return;

	++t->hit_count;
	t->hit_now = 1;
	t->ship = index;
	t->ship_i = i->i;

	t->xc = bfg->x + 5;
	t->yc = bfg->y + 5;
	t->xt = i->x + 8;
	t->yt = i->y + 8;
}

void CreateWallExplosion(int x, int y)
{
	TSHIP *j = PrepareFreeShip();
	j->state = SH_ACTIVE;
	j->i = 7;
	j->x = x;
	j->y = y;
	j->anim_speed = 6;
	j->anim_speed_cnt = 6;
	j->max_frame = 2;
	j->ai_type = AI_EXPLOSION;
}

void DoSmoke()
{
	if (!easy_level || ship_health > 1)
		return;

	static int smoke_counter = 0;
	if (--smoke_counter > 0)
		return;

	smoke_counter = 60;

	TSHIP *i = &Ships[0];
	TSHIP *j = PrepareFreeShip();

	j->state = SH_ACTIVE;
	j->i = 46;
	j->x = i->x + 8;
	j->y = i->y - 8;
	j->dy = -1;
	j->dx =
		FacingRight(i) ? -1 :
		(FacingLeft(i) ? 1 : 0);
	j->anim_speed = 4;
	j->anim_speed_cnt = j->anim_speed;
	j->max_frame = 4;
	j->ai_type = AI_SMOKE;
}

void DoEnemy(int f)
{
	TSHIP *i;

	i = &Ships[f];

	if(i->state == SH_DEAD) return;

	// if main ship is exploding, freeze other enemies except bridge and garage
	// it's not safe but ship dies after all and enemy data is reinitialized then
	if (f != 0 && Ships[0].ai_type == AI_EXPLOSION)
	{
		if(i->ai_type != AI_BRIDGE &&
			i->ai_type != AI_GARAGE &&
			i->ai_type != AI_HIDDEN_AREA_ACCESS &&
			i->ai_type != AI_SPARE_SHIP)
		{
			i->ai_type = AI_STATIC; // don't affect ship itself
		}
	}

	// do different ai types
	switch(i->ai_type)
	{
		case AI_STATIC: // breakable wall or non-moving enemy
			UpdateAnimation(f);
			break;
		case AI_RANDOM_MOVE:
_random_move_ai:
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1)
			{
				if(i->ai_update_cnt == 0)
				{
					i->dx = RandomInt() & 3;
					if(i->dx >= 2) i->dx = -1;

					i->dy = RandomInt() & 3;
					if(i->dy >= 2) i->dy = -1;

					i->ai_update_cnt = RandomInt() & 0x1f;
					if(i->ai_update_cnt < 15) i->ai_update_cnt = 15;
				}
				else
				{
					i->ai_update_cnt -= 1;
				}
			_optimize1:
				if(IsTouch(i->x + i->dx, i->y, f) == 0) i->x += i->dx; else i->ai_update_cnt = 0;
				if(IsTouch(i->x, i->y + i->dy, f) == 0) i->y += i->dy; else i->ai_update_cnt = 0;

			}
			break;
		case AI_KAMIKADZE:
			if (Ships[0].i == SHIP_TYPE_OBSERVER)
				goto _random_move_ai;

			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1)
			{
				if(i->ai_update_cnt == 0)
				{
					if(i->x > Ships[0].x)
					{
						i->dx = -1;
					}
					else
					{
						if(i->x < Ships[0].x) i->dx = 1; else i->dx = 0;
					}

					if(i->y > Ships[0].y)
					{
						i->dy = -1;
					}
					else {
						if(i->y < Ships[0].y) i->dy = 1; else i->dy = 0;
					}

					i->ai_update_cnt = 15;
				}
				else
				{
					i->ai_update_cnt -= 1;
				}

				goto _optimize1;
			}
			break;

		case AI_ELECTRIC_SPARKLE_VERTICAL:
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1)
			{
				if(i->dy == 0) i->dy = 1;
				if(IsTouch(i->x, i->y + i->dy, f) == 0) i->y += i->dy; else i->dy = -i->dy;
			}
			break;
		case AI_CEILING_CANNON: // ceiling cannon spawning kamikazes
			// if object is spawned - do nothig
			if(i->dx == 1) return;
			if(UpdateAnimation(f) == 1)
			{
				i->dx = 1;

				// spawn a new enemy
				{
					TSHIP *j = PrepareFreeShip();
					j->state = SH_ACTIVE;
					j->i = 34;
					j->x = i->x;
					j->y = i->y + 16;
					j->anim_speed = 4;
					j->anim_speed_cnt = j->anim_speed;
					j->max_frame = 3;
					j->ai_type = AI_KAMIKADZE;
					j->parent = f;
					PlaySoundEffect(SND_CANNON_SHOOT);
				}
			}
			break;
		case AI_HOMING_MISSLE:
			UpdateAnimation(f);

			if(i->x > 0)
			{
				i->x -= 2;
				IsTouch(i->x, i->y, f);
				if(i->x < Ships[0].x) return;
				if(i->y > Ships[0].y) i->y -= 1;
				if(i->y < Ships[0].y && i->y < 63) {if((RandomInt() & 1) == 1) i->y += 1;}
			}
			else
			{
				i->x = 296;
				i->y = RandomInt() & 63;
				if(i->i == 40) i->i = 41; else i->i = 40;
			}
			break;
		case AI_CANNON:
			if(i->x - 40 > Ships[0].x)
			{
				i->cur_frame = 0;
			}
			else
			{
				if(i->x + 40 < Ships[0].x) i->cur_frame = 2; else i->cur_frame = 1;
			}

			if((RandomInt() & 255) > 252)
			{
				TSHIP *j = PrepareFreeShip();
				if(j->state == SH_DEAD)
				{
					j->state = SH_ACTIVE;
					j->i = 43;
					if(i->cur_frame == 0) {j->x = i->x - 4; j->y = i->y + 4; j->dx = -1; j->dy = -1;}
					if(i->cur_frame == 1) {j->x = i->x + 6; j->y = i->y - 4; j->dx = 0; j->dy = -1;}
					if(i->cur_frame == 2) {j->x = i->x + 16; j->y = i->y + 4; j->dx = 1; j->dy = -1;}
					j->anim_speed = 4;
					j->anim_speed_cnt = j->anim_speed;
					j->ai_type = AI_BULLET;
					j->parent = f;
				}
			}
			break;
		case AI_ELECTRIC_SPARKLE_HORIZONTAL:
			UpdateAnimation(f);
			if(UpdateMoveSpeed(f) == 1)
			{
				if(i->dx == 0) i->dx = 1;
				if(IsTouch(i->x + i->dx, i->y, f) == 0) i->x += i->dx; else i->dx = -i->dx;
			}
			break;
		case AI_BONUS:
			if(UpdateAnimation(f) == 1)
			{
				static int yOffset[] = {
					-1, -1, -2, -2, -1,
					1, 1, 2, 2, 1 };

				i->y += yOffset[i->dy];
				i->dy = (i->dy + 1) % (sizeof(yOffset) / sizeof(yOffset[0]));
			}
			break;
		case AI_SMOKE:
			if(UpdateAnimation(f) == 1)
			{
				i->state = SH_DEAD;
			}
			else if (i->cur_frame % 2)
			{
				i->x += i->dx;
				i->y += i->dy;
			}
			break;
		case AI_EXPLOSION: // explosion, do one animation cycle and deactivate enemy entry
			if(UpdateAnimation(f) == 1)
			{
				if (i->explosion.bonus_type)
				{
					if (i->explosion.regenerate_bonus) {
						// Hit with laser, restore the other bonus.
						i->ai_type = AI_BONUS;
						i->state = SH_ACTIVE;
						switch (i->explosion.bonus_type)
						{
						case BONUS_FACEBOOK: i->i = BONUS_TWITTER; break;
						case BONUS_TWITTER: i->i = BONUS_FACEBOOK; break;
						default: i->i = i->explosion.bonus_type; break;
						}
						i->explosion.bonus_type = 0;
						i->explosion.regenerate_bonus = 0;
						i->dx = 0;
						i->dy = 0;
						i->max_frame = 0;
						i->cur_frame = 0;
						i->min_frame = 0;
						i->anim_speed = 4;
						return;
					}
					else {
						PlaySoundEffect(SND_BONUS);

						switch (i->explosion.bonus_type)
						{
							case BONUS_HP:
								// add HP
								++ship_health;
								if (ship_health > 3)
									ship_health = 3;
								break;

							case BONUS_FACEBOOK:
							case BONUS_TWITTER:
								HitTheBonus(i->explosion.bonus_type);
								break;
						}
					}

				}
				i->state = SH_DEAD;
				if(f == 0) RestartLevel();
				return;
			}
			break;
		case AI_BRIDGE: // bridge, appear if bonded ship, disappear otherwise
			{
				int a = 0;
				if(/*screen_bridge == 1 &&*/ player_attached == 1) a = 245;
				#if 0 //defined(__DINGUX__) || defined(__DINGOO__)
				{
					extern int level_cache_fl;
					level_cache_fl = 1;
				}
				#endif

				// seal or unseal the floor
				for(int f = 0; f <= 4; f++)
				{
					ScreenTilesBuffer[(i->y >> 3) * 40 + (i->x >> 3) + f] = a;
				}
			}
			break;

		case AI_GARAGE:
			{
				TSHIP *p = &Ships[0];
				if (p->state == SH_DEAD ||
					p->ai_type == AI_EXPLOSION)
					break;

				if (GarageShipIndex(i->i) != -1)
				{
					break;
				}

				int w, h;
				GetCurrentSpriteDimensions(p, &w, &h);

				if (!i->garage_inactive &&
					(p->x >= i->x) &&
					(p->x + w < i->x + GARAGE_WIDTH) &&
					(p->y >= i->y) &&
					(p->y + h < i->y + GARAGE_HEIGHT))
				{
					// Player ship is inside the garage, lets
					// change the ship if possible.
					TSHIP *r = NULL;
					for (int n = 2; n < SHIPS_NUMBER; ++n)
						if (Ships[n].ai_type == AI_SPARE_SHIP &&
							Ships[n].state == SH_ACTIVE)
						{
							r = Ships + n;
							break;
						}

					if (r)
					{
						// Swap data of the plyer ship and
						// the spare ship.
						TSHIP t = Ships[0];
						Ships[0] = *r;
						*r = t;

						int otherGarage = Ships[0].garage_index;

						Ships[otherGarage].garage_inactive = 1;

						SetGarageShipIndex(Ships[f].i, r->i);
						SetGarageShipIndex(Ships[otherGarage].i, -1);

						Ships[0].ai_type = 0;
						Ships[0].garage_index = -1;

						r->ai_type = AI_SPARE_SHIP;
						r->garage_index = f;

						// restore HP
						ship_health = 3;

						PlaySoundEffect(SND_CONTACT);
					}
				}
				else if (i->garage_inactive)
				{
					if (!IsOverlap(p, i))
					{
						i->garage_inactive = 0;
					}
				}
			}
			break;

		case AI_SPARE_SHIP:
			{
				const int speed = 1;
				int x, y;
				BestPositionInGarage(i, &x, &y);

				if (x > i->x) i->x += speed;
				else if (x < i->x) i->x -= speed;

				if (y > i->y) i->y += speed;
				else if (y < i->y) i->y -= speed;

				int middle_frame = (i->max_frame + i->min_frame) / 2;

				if (i->cur_frame > i->min_frame && i->cur_frame <= middle_frame)
					--(i->cur_frame);
				else if (i->cur_frame > middle_frame && i->cur_frame < i->max_frame)
					++(i->cur_frame);
			}
			break;

		case AI_BULLET: // bullet

			// random exploding
			if(i->y + 16 < Ships[i->parent].y && (RandomInt() & 63) == 1) {BlowUpEnemy(f); return; }

			i->x += i->dx;
			if(i->x < 0 || i->x > SCREEN_WIDTH) {BlowUpEnemy(f); return;}

			i->y += i->dy;
			if(i->y < 0) BlowUpEnemy(f);
			break;

		case AI_HOMING_SHOT: // missle shot by player.
			if (i->just_created == 1)
			{
				if (IsTouch(i->x, i->y, f))
				{
					BlowUpEnemy(f);
					return;
				}
			}

			// Calculate vertical speed.
			i->dy = 0;

			if (++i->ticks_passed > 10)
			{
				int best = -1;
				int dx_best = 0;

				for (int n = 2; n < SHIPS_NUMBER; ++n)
				{
					if (Ships[n].state != SH_ACTIVE ||
						(Ships[n].ai_type != AI_RANDOM_MOVE && Ships[n].ai_type != AI_KAMIKADZE))
						continue;

					if (Ships[n].i == 11)
						continue;

					int dx = Ships[n].x - i->x;
					if (i->cur_frame < 2)
					{
						// moving left.
						if (dx > 10 || dx < -110) continue;
						if (best == -1 || dx > dx_best)
						{
							best = n;
							dx_best = dx;
						}
					}
					else
					{
						// moving right.
						if (dx < 10 || dx > 110) continue;
						if (best == -1 || dx < dx_best)
						{
							best = n;
							dx_best = dx;
						}
					}
				}

				if (best != -1)
				{
					int dy = Ships[best].y - i->y;
					if (dy < 0) i->dy = -1;
					if (dy > 0) i->dy = 1;
				}
			}

			UpdateAnimation(f);

			i->x += i->dx;
			i->y += i->dy;

			if (/*i->x + i->dx < 0 ||
				i->x + i->dx >= SCREEN_WIDTH ||
				i->y < 0 ||
				i->y >= SCREEN_HEIGHT ||*/
				IsTouch(i->x, i->y, f))
			{
				i->state = SH_DEAD;
				return;
			}

			break;

		case AI_BFG_SHOT:
			UpdateAnimation(f);

			if (i->dx == 2) i->dx = 3;
			else if (i->dx == -2) i->dx = -3;
			else if (i->dx == 3) i->dx = 2;
			else if (i->dx == -3) i->dx = -2;

			// Calculate hit objects.
			{
				for (int n = 0; n < MAX_BFG_TARGETS; ++n)
					BfgTargets[n].hit_now = 0;

				for (int n = 2; n < SHIPS_NUMBER; ++n)
				{
					if (Ships[n].ai_type == AI_KAMIKADZE || Ships[n].ai_type == AI_RANDOM_MOVE)
					{
						if (ShipsDistance(i, Ships + n) < BFG_KILL_DISTANCE)
						{
							AddBfgTarget(n, f);
						}
					}
				}

				for (int n = 0; n < MAX_BFG_TARGETS; ++n)
				{
					if (BfgTargets[n].hit_count)
					{
						if (BfgTargets[n].hit_now)
						{
							if (BfgTargets[n].hit_count > BFG_KILL_TIME)
							{
								BlowUpEnemy(BfgTargets[n].ship);
							}
						}
						else
						{
							memset(BfgTargets + n, 0, sizeof(TBFGTARGET));
						}
					}
				}
			}

		case AI_SHOT: // bullet shot by a player
			if (i->just_created)
			{
				i->just_created = 0;
				if (IsTouch(i->x, i->y, f))
				{
					BlowUpEnemy(f);
					return;
				}
			}

			i->x += i->dx;
			i->y += i->dy;

			if (i->x + i->dx < 0 ||
				i->x + i->dx >= SCREEN_WIDTH ||
				i->y < 0 ||
				i->y >= SCREEN_HEIGHT ||
				IsTouch(i->x, i->y, f))
			{
				BlowUpEnemy(f);
				return;
			}
			break;

		case AI_ELEVATOR: // elevator
			if (player_attached == 1)
			{
				// start to lift only when ship and base are standing on the elevator
				// ugly, improve in future
				//if((i->x == 256 && Ships[1].x >= 260) || (i->x == 16 && Ships[1].x <= 20))
				if(i->x == Ships[1].x - 4)
				{
					static int el_phase = 0;

					elevator_flag = 1;

					if (el_phase == 0)
					{
						// when starting to lift up - unseal the floor
						// ugly, maybe change in future
						if(i->y == 120)
						{
							#if 0 //defined(__DINGUX__) || defined(__DINGOO__)
							extern int level_cache_fl;
							level_cache_fl = 1; // doing some magic...
							#endif
							// unseal the floor
							for(int j = 0; j <= 5; j++)
							{
								ScreenTilesBuffer[(i->y >> 3) * 40 + (i->x >> 3) + j] = 0;
							}
						}

						// upper limit of the screen is reached
						if(Ships[0].y == 0)
						{
							el_phase = 1;

							int sx, sy;
							GetCurrentSpriteDimensions(&Ships[0], &sx, &sy);

							Ships[0].y = 112 - sy;
							Ships[1].y = 112;

							ChangeScreen(F_UP);
							base_cur_screen = ship_cur_screen;
							InitNewScreen();

							// now i is invalid, because InitNewScreen reenables enemies
							// spawn new elevator
							i = PrepareFreeShip();
							i->state = SH_ACTIVE;
							i->i = 21;
							i->x = Ships[1].x - 4;
							i->y = 128;
							i->ai_type = AI_ELEVATOR;
							goto _here;
						}
					}
					else
					{
						// if elevator is done lifting
						if(Ships[1].y == 104)
						{
							el_phase = 0;

							// seal the floor!
							for(int i = 0; i <= 5; i++)
							{
								#if 0 //defined(__DINGUX__) || defined(__DINGOO__)
								extern int level_cache_fl;
								level_cache_fl = 1; // doing some magic...
								#endif
								ScreenTilesBuffer[((Ships[1].y + 16) >> 3) * 40 + ((Ships[1].x - 4) >> 3) + i] = 245;
							}

							if(ship_cur_screen != 69)
							{
								game_level += 1;
								memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
								PublishScore();
								GameLevelUp();
							}
							base_level_start = ship_cur_screen;

							// destroy elevator or it will roll forever
							// but if not screen 69
							if(base_cur_screen != 69)
							{
								for(int j = 2; j <= SHIPS_NUMBER-2; j++)
								{
									if(Ships[j].ai_type == AI_ELEVATOR) Ships[j].state = SH_DEAD;
								}
							}

							elevator_flag = 0;
							ship_health = 3;

							goto _here;
						}
					}

					Ships[0].y -= 1;
					Ships[1].y -= 1;
					i->y -= 1;

				_here:
					if (elevator_flag) PlaySoundEffect(SND_ELEVATOR);
					else StopSoundEffect(SND_ELEVATOR);
				}
			}
			break;
	}
}

void InitShip()
{
	memcpy(garage_data, main_garage_data, sizeof(garage_data));

	memset(Ships, 0, sizeof(TSHIP) * SHIPS_NUMBER);

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

	// flying ship data
	Ships[0].state = SH_ACTIVE;
	Ships[0].i = GetPlayerShipIndex();
	Ships[0].x = 148 + ShipBaseOffset();
	Ships[0].y = 68;

	switch (Ships[0].i)
	{
	case SHIP_TYPE_LASER:
	case SHIP_TYPE_MACHINE_GUN:
	case SHIP_TYPE_ROCKET_LAUNCHER:
		Ships[0].max_frame = 6;
		Ships[0].min_frame = 0;
		break;

	case SHIP_TYPE_OBSERVER:
		Ships[0].max_frame = 3;
		Ships[0].min_frame = 1;
		break;

	case SHIP_TYPE_BFG:
		Ships[0].min_frame = 0;
		Ships[0].max_frame = 4;
		break;
	}

	Ships[0].cur_frame = ((game_level & 1) == 0 ? Ships[0].max_frame : Ships[0].min_frame);
	Ships[0].anim_speed = 1;
	Ships[0].anim_speed_cnt = 1;
}

int GetFreeEnemyIndex()
{
	for(int i = 2; i <= SHIPS_NUMBER-1; i++)
	{
		if(Ships[i].state == SH_DEAD) return i; // and ai_type should be zero!
	}

	return SHIPS_NUMBER-1;
}

TSHIP *PrepareFreeShip()
{
	TSHIP *ship = Ships + GetFreeEnemyIndex();
	memset(ship, 0, sizeof(TSHIP));
	return ship;
}

void BestPositionInGarage(TSHIP *ship, int *x, int *y)
{
	int cxShip, cyShip;
	GetCurrentSpriteDimensions(ship, &cxShip, &cyShip);

	if (ship->garage_index < 0 || ship->garage_index >= SHIPS_NUMBER)
	{
		// actually, should not happen.
		*x = ship->x;
		*y = ship->y;
	}
	else
	{
		TSHIP *garage = &Ships[ship->garage_index];
		*x = garage->x + ((GARAGE_WIDTH - cxShip) >> 1);
		*y = garage->y + ((GARAGE_HEIGHT - cyShip) >> 1);
	}
}

void InitGaragesForNewGame()
{
	memset(garage_data, 0, sizeof(garage_data));

	int n = 0;
	garage_data[n  ][0] = 100;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 101;
	garage_data[n++][1] = SHIP_TYPE_MACHINE_GUN;

	garage_data[n  ][0] = 110;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 111;
	garage_data[n++][1] = SHIP_TYPE_ROCKET_LAUNCHER;

	garage_data[n  ][0] = 120;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 121;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 122;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 123;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 124;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 130;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 131;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 190;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 191;
	garage_data[n++][1] = SHIP_TYPE_BFG;

	memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
}

void SetGarageShipIndex(int garageId, int shipIndex)
{
	for (int i = 0; i < MAX_GARAGES; ++i)
	{
		if (garage_data[i][0] == garageId)
		{
			garage_data[i][1] = shipIndex;
		}
	}
}

int GarageShipIndex(int garageId)
{
	for (int i = 0; i < MAX_GARAGES; ++i)
	{
		if (garage_data[i][0] == garageId)
		{
			return garage_data[i][1];
		}
	}
	return -1;
}

int IsParked(int ship_type)
{
	for (int i = 0; i < MAX_GARAGES; ++i)
	{
		if (garage_data[i][0] && garage_data[i][1] == ship_type)
		{
			return 1;
		}
	}
	return 0;
}

int GetPlayerShipIndex()
{
	if (!IsParked(SHIP_TYPE_LASER))
		return SHIP_TYPE_LASER;

	if (!IsParked(SHIP_TYPE_MACHINE_GUN))
		return SHIP_TYPE_MACHINE_GUN;

	if (!IsParked(SHIP_TYPE_ROCKET_LAUNCHER))
		return SHIP_TYPE_ROCKET_LAUNCHER;

	if (!IsParked(SHIP_TYPE_BFG))
		return SHIP_TYPE_BFG;

	return SHIP_TYPE_OBSERVER;
}

void InitEnemies()
{
	unsigned char *p;
	TSHIP *en;

	memset(&Ships[2] , 0, sizeof(TSHIP) * (SHIPS_NUMBER - 2));

	p = SCREENINFOS[ship_cur_screen] + 5;

	for(int i = *(p-1); i >= 1; i--)
	{
		if (*(p + 1) == BONUS_FACEBOOK || *(p + 1) == BONUS_TWITTER)
		{
			if (!sn_enabled || game_mode == GM_DEMO || base_cur_screen < ship_cur_screen)
				goto __skip_enemy;
		}

		en = PrepareFreeShip();
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
		en->move_speed = 1; // standard
		en->move_speed_cnt = en->move_speed;

		if (en->ai_type == AI_GARAGE)
		{
			en->i = *p;

			int iShip = GarageShipIndex(*p);
			if (iShip != -1)
			{
				// Find which type of the ship is supposed to be here,
				// create the ship in the best position inside it.
				TSHIP *ship = PrepareFreeShip();
				ship->state = SH_ACTIVE;
				ship->i = iShip;
				ship->ai_type = AI_SPARE_SHIP;
				ship->garage_index = IndexOf(en);
				switch (iShip)
				{
				case SHIP_TYPE_LASER:
				case SHIP_TYPE_MACHINE_GUN:
				case SHIP_TYPE_ROCKET_LAUNCHER:
					ship->max_frame = 6;
					ship->min_frame = 0;
					break;
				case SHIP_TYPE_OBSERVER:
					ship->max_frame = 3;
					ship->min_frame = 1;
					ship->cur_frame = 1;
					break;
				case SHIP_TYPE_BFG:
					ship->max_frame = 4;
					ship->min_frame = 0;
					break;
				}
				BestPositionInGarage(ship, &ship->x, &ship->y);
			}
		}
		else if (en->ai_type == AI_HIDDEN_AREA_ACCESS)
		{
			en->dx = *(p + 4);
			en->dy = *(p + 5);

			if (hidden_level_entered)
			{
				DestroyHiddenAreaAccess(en, 0);
			}
		}

__skip_enemy:;
		p += 8;
	}

	screen_procedure = *(p++);
	cur_screen_bonus = *p;
}

void InitNewScreen()
{
	UnpackLevel();

	InitEnemies();

	ReEnableBase();

	laser_dir = 0;

	if (ship_cur_screen == 92)
	{
		// Memorize that we entered the hidden area once.
		hidden_level_entered = 1;
	}
	else if (hidden_level_entered && ship_cur_screen == 1)
	{
		// Save game, cause we are back from the underggroung (most probably, heh).
		memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
		PublishScore();

	}

	CleanupBfg();
}

int GameLevelFromScreen(int screen)
{
	int levels[10] = {
		8, 15, 22, 29, 36, 43, 50, 55, 62, 70
	};

	for (int i = 0; i < 10; ++i) {
		if (levels[i] > screen)
			return i + 1;
	}

	return 10;
}

void InitNewGame()
{
	ship_fuel = 5000;
	ship_lives = 10;
	ship_health = 3;
	ship_score = 0;

	ship_cur_screen = GAME_START_SCREEN;
	base_cur_screen = GAME_START_SCREEN;
	base_level_start = GAME_START_SCREEN;
	game_level = GameLevelFromScreen(GAME_START_SCREEN);

	player_attached = 0;
	hidden_level_entered = 0;

	InitGaragesForNewGame();
	InitShip();
	InitNewScreen();

	for(int f = 0; f <= 6; f++)
	{
		PutStream(0, STATUSBAR1[f * 43 + 1] * 8, &STATUSBAR1[f * 43 + 2]);
	}
}

int UpdateLives()
{
	ship_lives -= 1;
	ship_health = 3;

	// Just in case they are playing.
	StopSoundEffect(SND_ELEVATOR);
	StopSoundEffect(SND_MOVE);
	PublishScore();

	if(ship_lives == 0)
	{
		SetGameMode(GM_GAMEOVER);
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

	// health bar.
	string_buffer[2] = 0;
	for (int y = 0; y < 3; ++y)
	{
		string_buffer[1] = string_buffer[0] = (ship_health > y) ? 84 : 88;
		PutStream(8*19, 8*(22 - y), (unsigned char *)&string_buffer[0]);
	}

	// laser
	for(int i = 0; i <= 31; i++)
	{

		unsigned char c = ((i < (laser_overload >> 3)) ? 0x28 : 0);

		*(pScreenBuffer + i + 192 + 162 * SCREEN_WIDTH) = c;
		*(pScreenBuffer + i + 192 + 163 * SCREEN_WIDTH) = c;
		*(pScreenBuffer + i + 192 + 164 * SCREEN_WIDTH) = c;
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
	if(ticks_for_splash == 0) {
		for(int i = 0; i <= 199; i++) {
			PutGeneric(0, i * 2, SCREEN_WIDTH, 1, &splash_screen[i*80]);
			PutGeneric(0, i * 2 + 1, SCREEN_WIDTH, 1, &splash_screen[i*80+8192]);
		}
	}

	ticks_for_splash += 1;
	if(ticks_for_splash > 350) SetGameMode(GM_TITLE);
	if(LM_AnyKey() == 1) {LM_ResetKeys(); SetGameMode(GM_TITLE);}
}

int ticks_before_demo = 0;

void ResetDemoTicksCounter()
{
	ticks_before_demo = 0;
}

void DoTitle()
{
	if(title_start_flag == 0)
	{
		memset(pScreenBuffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
		ship_cur_screen = 0;
		title_start_flag = 1;
		InitNewScreen();
		BlitLevel();
		PutSprite(50*4, 108, *(pSprites256[45] + 0));
		PutString(76, 88, "ESPACIO PARA COMENZAR");
		PutString(60, 24, "ORIGINAL GAME: PEDRO RUIZ");
		PutString(76, 36, "REMAKE: DMITRY SMAGIN");
		PutString(140, 44, "ALEXEY PAVLOV");
		PutString(60, 56, "MUSIC AND SFX: MARK BRAGA");
		ResetDemoTicksCounter();
	}

	RotateLogo();

	ticks_before_demo++;

	// wait some time before switching to demo mode
	if(ticks_before_demo >= 3660) // wait 1 min
	{
		easy_level = 0;
		ResetDemo();
		SetGameMode(GM_DEMO);
		LM_ResetKeys();
		InitNewGame();
		return;
	}

	// exit to os
	if(Keys[SC_ESCAPE] == 1)
	{
		SetGameMode(GM_EXIT);
		return;
	}

	// start
	if(Keys[SC_SPACE] == 1 || Keys[SC_ENTER] == 1)
	{
		SetGameMode(GM_GAME);
		LM_ResetKeys();
		InitNewGame();
	}
}

void DoWinScreen()
{
	static int x_string = 0;
	static int win_ticks = 0;
	static char win_string[410] = ""
	"                                        "
	"ATENCION    ATENCION     TRANSMISION A LA NAVE EXPLORER           "
	"HAS CUMPLIDO TU ULTIMA MISION Y DEBES RETORNAR AL PLANETA NOVA DE LA GALAXIA TRAION"
	"                                        "
	"TU LUCHA NO HA SIDO EN VANO PUES LA LEJANA COLONIA DEL IMPERIO LLAMADA TIERRA HA SIDO "
	"LIBERADA DE LOS INVASORES Y PUEDE SER HABITADA DE NUEVO               REPITO  MENSAJE";

	if (youwin_start_flag == 0)
	{
		memset(pScreenBuffer + 144 * SCREEN_WIDTH, 0, SCREEN_WIDTH * 56);
		youwin_start_flag = 1;
		x_string = 0;
		win_ticks = 0;
	}
	else
	{
		win_ticks++;
	}

	PutString(0 - x_string % 8, 20*8, &win_string[0] + x_string/8);

	if (x_string/8 >= sizeof(win_string))
		x_string = 0;
	else
		x_string += 1;

	if (LM_AnyKey() == 1 && win_ticks > 300) // Ignore input first 5 seconds.
	{
		SetGameMode(GM_TITLE);
		LM_ResetKeys();
	}
	else
	{
		// Update animations and screen.
		GKeys[KEY_RIGHT] = (Ships[0].x < 93) ? 1 : 0;
		GKeys[KEY_LEFT] = 0;
		GKeys[KEY_UP] = (Ships[0].x < 93 && Ships[0].y > 40 ) ? 1 : 0;
		GKeys[KEY_DOWN] = 0;
		GKeys[KEY_FIRE] = 0;

		DoShip();

		for(int i = 2; i <= SHIPS_NUMBER-1; i++)
			UpdateAnimation(i);

		if (!frame_skip)
			RenderGame(0);
	}
}

void DoKeys()
{
	// if not demo mode
	if(game_mode != GM_DEMO)
	{
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
	if(laser_dir != 0)
	{
		DrawLine(x_start, ly, x_end, ly, 7);
	}
}

void BlitBfg()
{
	if (bfg_on)
	{
		for (int n = 0; n < MAX_BFG_TARGETS; ++n)
		{
			if (BfgTargets[n].ship)
			{
				unsigned char color = 10;

				if (BfgTargets[n].hit_count < BFG_KILL_TIME / 2)
					color = 2;

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

void BlitEnemies()
{
	for(int i = 0; i <= SHIPS_NUMBER-1; i++)
	{
		if (Ships[i].ai_type == AI_GARAGE)
		{
#ifdef _DEBUG
			DrawRect(
				Ships[i].x, Ships[i].y,
				GARAGE_WIDTH, GARAGE_HEIGHT,
				Ships[i].garage_inactive ? 10 : 28);
#endif
		}
		else if (Ships[i].ai_type == AI_HIDDEN_AREA_ACCESS)
		{
#ifdef _DEBUG
			if (Ships[i].state == SH_ACTIVE)
			{
				DrawRect(
					Ships[i].x, Ships[i].y,
					Ships[i].dx, Ships[i].dy,
					10);
			}
#endif
		}
		else if (Ships[i].state != SH_DEAD && Ships[i].ai_type != AI_BRIDGE)
		{
			PutSprite(Ships[i].x, Ships[i].y, *(pSprites256[Ships[i].i] + Ships[i].cur_frame));
		}
	}
}

void BlitEnemyOutlines()
{
	unsigned char shadow = GetScreenDrawInfo(ship_cur_screen)->shadow;
	for(int i = 0; i <= SHIPS_NUMBER-1; i++)
	{
		if (Ships[i].ai_type == AI_BRIDGE && !player_attached)
			continue;

		if (Ships[i].state == SH_DEAD ||
			Ships[i].ai_type == AI_EXPLOSION ||
			Ships[i].ai_type == AI_GARAGE ||
			Ships[i].ai_type == AI_SMOKE ||
			Ships[i].ai_type == AI_SHOT ||
			Ships[i].ai_type == AI_BFG_SHOT ||
			Ships[i].ai_type == AI_HIDDEN_AREA_ACCESS)
			continue;

		if ((Ships[i].ai_type == AI_ELECTRIC_SPARKLE_HORIZONTAL ||
			Ships[i].ai_type == AI_ELECTRIC_SPARKLE_VERTICAL) && Ships[i].i != 11)
		   continue;

		PutSpriteOutline(Ships[i].x, Ships[i].y, *(pSprites256[Ships[i].i] + Ships[i].cur_frame), shadow);
	}
}

unsigned char *pLightBuffer = 0;
int numLights = 0;
Light lights[MAX_LIGHTS];

void AddLight(int x, int y, int radius, unsigned char r, unsigned char g, unsigned char b)
{
	if (numLights >= MAX_LIGHTS)
		return;

	Light *l = &lights[numLights++];
	l->x = x;
	l->y = y;
	l->radius = radius;
	l->b = b;
	l->g = g;
	l->r = r;
}

int ExplosionRadius(TSHIP *ship, int maxRadius)
{
	return maxRadius * cos((3.14159 * 0.5 * ship->cur_frame) / ship->max_frame);
}

void CastLights()
{
	numLights = 0;

	// Laser.
	if (laser_dir)
	{
		AddLight((x_start + x_end) / 2, ly, 60, 50, 50, 50);
	}

	// Enemies.
	for(int i = 0; i <= SHIPS_NUMBER-1; i++)
	{
		if(Ships[i].state != SH_DEAD && Ships[i].ai_type != AI_BRIDGE)
		{
			switch (Ships[i].ai_type)
			{
			case AI_ELECTRIC_SPARKLE_HORIZONTAL:
			case AI_ELECTRIC_SPARKLE_VERTICAL:
				AddLight(Ships[i].x + 8, Ships[i].y + 8, 50, 200, 50, 50);
				break;

			case AI_SHOT:
				AddLight(Ships[i].x + 2, Ships[i].y, 20, 100, 50, 50);
				break;

			case AI_BFG_SHOT:
				AddLight(Ships[i].x + 5, Ships[i].y + 5, 16, 90, 200, 90);
				break;

			case AI_HOMING_SHOT:
				if (Ships[i].cur_frame < 2)
					AddLight(Ships[i].x + 14, Ships[i].y + 4, 20, 150, 110, 100);
				else
					AddLight(Ships[i].x, Ships[i].y + 4, 20, 150, 110, 100);
				break;

			case AI_EXPLOSION:
				AddLight(Ships[i].x + 10, Ships[i].y + 10, ExplosionRadius(&Ships[i], 80), 240, 100, 0);
				break;

			case AI_BONUS:
				if (Ships[i].i == BONUS_HP) AddLight(Ships[i].x + 5, Ships[i].y + 5, 25, 200, 80, 80);
				break;

			case AI_SMOKE:
				AddLight(Ships[i].x + 8, Ships[i].y + 8, ExplosionRadius(&Ships[i], 40), 100, 70, 0);
				break;
			}
		}
	}
}

void BlitNonAmbientEnemies()
{
	for(int i = 0; i <= SHIPS_NUMBER-1; i++)
	{
		if (Ships[i].state != SH_DEAD &&
			Ships[i].ai_type != AI_BRIDGE &&
			Ships[i].ai_type != AI_ELECTRIC_SPARKLE_HORIZONTAL &&
			Ships[i].ai_type != AI_ELECTRIC_SPARKLE_VERTICAL &&
			Ships[i].ai_type != AI_SMOKE &&
			Ships[i].ai_type != AI_GARAGE &&
			Ships[i].ai_type != AI_HIDDEN_AREA_ACCESS &&
			Ships[i].ai_type != AI_EXPLOSION)
			PutSprite(Ships[i].x, Ships[i].y, *(pSprites256[Ships[i].i] + Ships[i].cur_frame));
	}
}

void RenderGame(int renderStatus)
{
	if (modern_background) {
		BlitBackground(); // blit background
		BlitLevelOutlines();
		BlitEnemyOutlines(); // draw moving objects' outlines (shadows)
	}
	else {
		EraseBackground();
	}

	BlitLevel(); // blit walls
	BlitBfg();
	BlitEnemies(); // draw all enemies and cannon+base
	BlitLaser(); // don't forget laser

	if (renderStatus)
		BlitStatus(); // draw score etc

	if (pLightBuffer)
	{
		// Render light map
		unsigned char *mainSB = pScreenBuffer;
		pScreenBuffer = pLightBuffer;
		memset(pScreenBuffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
		BlitLevel();
		BlitNonAmbientEnemies();
		CastLights();
		pScreenBuffer = mainSB;
	}
}

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
			if(PlayDemo() == 1 || LM_AnyKey() == 1)
			{
				SetGameMode(GM_TITLE);
				LM_ResetKeys();
				ship_score = 0;
				break;
			}

		case GM_GAME:

			DoKeys();

			if(GKeys[KEY_PAUSE] == 1)
			{
				PutString(8*17, 8*17, "PAUSA");
				SetGameMode(GM_PAUSE);
				Keys[SC_ENTER] = 0;
				break;
			}

#ifdef _DEBUG
			{
				static char screen_nr_text[16];
				sprintf(screen_nr_text, "SCREEN %i", ship_cur_screen);
				PutString(8*17, 8*17, screen_nr_text);
			}
#endif

			RecordDemo();

			if(Keys[SC_ESCAPE] == 1)
			{
				SetGameMode(GM_TITLE);
				LM_ResetKeys();
				break;
			}

			// win the game:
			if(screen_procedure == 3 /*&& base_cur_screen >= 70*/)
			{
				SetGameMode(GM_YOUWIN);

				LM_ResetKeys();
				PublishScore();
				return;
			}

			// do enemies
			for(int i = 2; i <= SHIPS_NUMBER-1; i++)
				DoEnemy(i);

			DoShip();
			DoBase();
			DoSmoke();

			if (Ships[0].state == SH_ACTIVE)
			{
				switch (Ships[0].i)
				{
				case SHIP_TYPE_ROCKET_LAUNCHER: DoRocketLauncher(); break;
				case SHIP_TYPE_MACHINE_GUN: DoMachineGun(); break;
				case SHIP_TYPE_BFG: DoBFG(); break;
				}

				// Always do laser.
				DoLaser();
			}

			if (!frame_skip)
				RenderGame(1);
			break;
		case GM_PAUSE:
				DoKeys();
				if(GKeys[KEY_PAUSE] == 1) {
					PutString(8*17, 8*17, "     ");
					SetGameMode(GM_GAME);
					Keys[SC_ENTER] = 0;
				}
			break;
		case GM_GAMEOVER:
			if(LM_AnyKey() == 1) {InitNewGame(); LM_ResetKeys(); SetGameMode(GM_TITLE);}
			break;
		case GM_YOUWIN:
			DoWinScreen();
			break;
		case GM_SPLASH:
			DoSplash();
			break;
	}

}

int GameMode()
{
	return game_mode;
}

void SetEasyLevel(int level)
{
	easy_level = level;
}

#ifndef __APPLE__

void HitTheBonus(int param)
{
}

void PublishScore()
{
}

void GameLevelUp()
{
}

int main(int argc, char *argv[])
{
	static char infostring[16] = "FPS: ";
	static int next_game_tick = 0;
	static int sleep_time = 0, frames = 0, frame_end = 0, frame_start = 0;
	static int show_fps = 0, max_frameskip = 0;

	if(LM_Init(&pScreenBuffer) == 0) return -1;
	LM_SND_Init();

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

	LM_SND_Deinit();
	LM_Deinit();

	return 0;
}

#else // __APPLE__

void SingleMainStep(float delta)
{
#if (1)
	const float frameDuration = 0.017;
	static float timePassed = 0;

	if (delta < 1)
	{
		timePassed += delta;

		if (timePassed > frameDuration) {
			timePassed -= frameDuration;
			while (timePassed > frameDuration) {
				timePassed -= frameDuration;
				frame_skip = 1;
				DoGame();
			}
			timePassed = 0;
			frame_skip = 0;
		}
	}
#endif

	DoGame();
}

#endif // __APPLE__

void EnableSocialNetworkIcon(int enable)
{
	sn_enabled = enable;
}

void SetModernBackground(int imodern)
{
	modern_background = imodern;
}

int CurrentLevel()
{
	return game_level;
}

int CurrentPoints()
{
	return ship_score;
}

void LoadGame(TGAMEDATA *data)
{
	player_attached = 0;
	screen_bridge = 0;
	laser_overload = 0;
	ticks_for_splash = 0;
	SetGameMode(GM_GAME);
	elevator_flag = 0;

	easy_level = data->easy_level;

	hidden_level_entered = data->hidden_level_entered;
	ship_fuel = data->fuel;
	ship_lives = data->num_lives;
	ship_health = ship_health;
	ship_score = data->ship_score;

	ship_cur_screen = data->base_level;
	base_cur_screen = data->base_level;
	base_level_start = data->base_level;
	game_level = GameLevelFromScreen(data->base_level);

	memcpy(garage_data, data->garages, sizeof(garage_data));
	memcpy(main_garage_data, data->garages, sizeof(main_garage_data));

	InitShip();
	InitNewScreen();

	for(int f = 0; f <= 6; f++)
	{
		PutStream(0, STATUSBAR1[f * 43 + 1] * 8, &STATUSBAR1[f * 43 + 2]);
	}
}

void SaveGame(TGAMEDATA *data)
{
	data->ship_score = ship_score;
	data->base_level = base_level_start;
	data->num_lives = ship_lives;
	data->hidden_level_entered = hidden_level_entered;
	data->fuel = ship_fuel;
	data->health = ship_health;
	data->easy_level = easy_level;
	memcpy(data->garages, main_garage_data, sizeof(main_garage_data));
}

void ResetGame(int gameMode)
{
	player_attached = 0;
	base_level_start = 1;
	screen_bridge = 0;
	game_level = 1;
	ship_fuel = 5000;
	ship_lives = 10;
	hidden_level_entered = 0;
	ship_health = 3;
	ship_score = 0;
	laser_overload = 0;
	ticks_for_splash = 0;
	SetGameMode(gameMode);
	elevator_flag = 0;

	InitNewGame();
}

int CurrentShipType()
{
	return Ships[0].i;
}

