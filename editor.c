/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS for(int A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <SDL/SDL.h>
#include "input.h"
#include "sprites.h"
#include "engine.h"
#include "object.h"
#include "object_garage.h"
#include "sprites.h"
#include "editor.h"
#include "room.h"

#define EDIT_MAP	0
#define EDIT_ROOMVIEW	1
#define EDIT_ROOM	2
#define EDIT_MACROTILE	3
#define EDIT_SPRITESET	4

/* Scary macro to make code more readable */
#define ON_PRESS(SC, COND, ACT) \
if (Keys[SC] && (COND)) { input_reset(); reinit = 1; ACT; }

#define ELSE(COND, ACT) \
else if (COND) {ACT; input_reset(); reinit = 1; }

static int reinit = 1;
static int cur_room = 1;
static int cur_patternset = 0;
static int cur_sprite = 0;
static int old_editmode = EDIT_MAP;
static int editmode = EDIT_ROOMVIEW;
static int cur_pattern = 0;
static int cur_mapx = 0, cur_mapy = 11;

/* FIXME: eliminate later */
extern SDL_Surface *small_screen;

void ReDraw(int roomnum)
{
	ClearScreen();
	UnpackRoom(game->world, roomnum);
	BlitRoom();
	InitGaragesForNewGame();
	GarageRestore();
	gObj_DestroyAll();
	InitEnemies(roomnum);
	BlitEnemies();
}

static void ShowMap()
{
	SDL_Rect dst;
	/* Backup small_screen */
	SDL_Surface *old_screen = small_screen;

	/* Reassign it temporarily */
	small_screen =
		SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, ACTION_SCREEN_HEIGHT,
				     GAME_SCREEN_BPP, 0, 0, 0, 0);

	for (int y = 0; y < 5; y++)
		for (int x = 0; x < 3; x++) {
			int roomnum = getscreen(cur_mapx + x - 1, cur_mapy + y - 2);

			if (roomnum) {
				ReDraw(roomnum);

				dst.x = 107 * x;
				dst.y = 46 * y;
				dst.w = SCREEN_WIDTH/3;
				dst.h = ACTION_SCREEN_HEIGHT/3;
				SDL_SoftStretch(small_screen, NULL, old_screen, &dst);
			}
		}

	SDL_FreeSurface(small_screen);
	small_screen = old_screen;

	DrawRect(107, 46*2, 106, 45, RGB(255, 0, 255));
}

static void ShowMapInfo()
{
	static char string[32];
	int roomnum = getscreen(cur_mapx, cur_mapy);

	sprintf(string, "ROOM    %03i:%03i", roomnum, game->world->room_num - 1);
	PutString(0*8, 29*8, string);
	sprintf(string, "X %03i", cur_mapx);
	PutString(18*8, 29*8, string);
	sprintf(string, "Y %03i", cur_mapy);
	PutString(26*8, 29*8, string);
}

static void MapEdit()
{
	if (reinit) {
		ClearScreen();
		ShowMap();
		ShowMapInfo();
		reinit = 0;
	}

	if (Keys[SC_ENTER]) {
		input_reset(); reinit = 1;
		old_editmode = editmode; editmode = EDIT_ROOM;
		cur_room = getscreen(cur_mapx, cur_mapy);
	}

	ON_PRESS(SC_LEFT, cur_mapx > 0, cur_mapx--);
	ON_PRESS(SC_RIGHT, cur_mapx < game->world->mapw - 1, cur_mapx++);
	ON_PRESS(SC_UP, cur_mapy > 0, cur_mapy--);
	ON_PRESS(SC_DOWN, cur_mapy < game->world->maph - 1, cur_mapy++);
	ON_PRESS(SC_ESCAPE, 1, SetGameMode(GM_TITLE));
}

static void ShowViewModeInfo()
{
	static char string[32];
	PATTERNSET *patternset = game->world->patternset + cur_patternset;
	unsigned short *data = (unsigned short *)patternset->data;

	for (int y = 0; y < patternset->ys; y++) {
		for (int x = 0; x < patternset->xs; x++) {
			PutTileI(x*8 + 132, y*8 + 22*8, *data++);
		}
	}

	PutSpriteI(30*8, 22*8, cur_sprite, 0);

	sprintf(string, "ROOM    %03i:%03i", cur_room, game->world->room_num - 1);
	PutString(0*8, 21*8, string);
	sprintf(string, "PTRNSET %03i:%03i", cur_patternset, game->world->patternset_num - 1);
	PutString(0*8, 22*8, string);
	sprintf(string, "SPRTSET %03i:%03i", cur_sprite, SPRITE_NUMBER - 1);
	PutString(0*8, 23*8, string);

	PutString(16*8, 20*8, "VIEW MODE");
}

static void ShowEditModeInfo()
{
	static char string[32];
	ROOM *room = game->world->room + cur_room;
	PATTERN *pattern = room->pattern + cur_pattern;
	PATTERNSET *patternset = game->world->patternset + pattern->index;

	PutString(16*8, 20*8, "ROOM EDIT");

	sprintf(string, "PATTERN %03i:%03i", cur_pattern, room->pattern_num - 1);
	PutString(0*8, 21*8, string);

	sprintf(string, "X %03i", pattern->x * 8);
	PutString(0*8, 22*8, string);
	sprintf(string, "Y %03i", pattern->y * 8);
	PutString(0*8, 23*8, string);
	sprintf(string, "I %03i:%03i", pattern->index, game->world->patternset_num - 1);
	PutString(0*8, 24*8, string);

	DrawRect(pattern->x*8,
		 pattern->y*8,
		 patternset->xs * 8 - 1,
		 patternset->ys * 8 - 1, RGB(255, 0, 255));
}

static void RoomView()
{
	if (reinit) {
		ReDraw(cur_room);
		ShowViewModeInfo();
		reinit = 0;
	}

	ON_PRESS(SC_ENTER, 1, old_editmode = editmode; editmode = EDIT_ROOM; cur_pattern = 0);
	ON_PRESS(SC_ESCAPE, 1, SetGameMode(GM_TITLE));
	ON_PRESS(SC_Z, cur_patternset > 0, cur_patternset--);
	ON_PRESS(SC_X, cur_patternset < game->world->patternset_num - 1,
		cur_patternset++);
	ON_PRESS(SC_C, cur_sprite > 0, cur_sprite--);
	ON_PRESS(SC_V, cur_sprite < SPRITE_NUMBER - 1, cur_sprite++);
	ON_PRESS(SC_LEFT, cur_room > 0, cur_room--);
	ON_PRESS(SC_RIGHT, cur_room < game->world->room_num - 1,
		cur_room++);
}

static void RoomEdit()
{
	ROOM *room = game->world->room + cur_room;
	PATTERN *pattern = room->pattern + cur_pattern;

	if (reinit) {
		ReDraw(cur_room);
		ShowEditModeInfo();
		reinit = 0;
	}

	ON_PRESS(SC_ESCAPE, 1, editmode = old_editmode);
	ON_PRESS(SC_Z, pattern->index > 0, pattern->index--);
	ON_PRESS(SC_X, pattern->index < game->world->patternset_num - 1,
		pattern->index++)

	if (Keys[SC_LEFT]) {
		ON_PRESS(SC_SPACE, pattern->x > 0,
			pattern->x--; Keys[SC_SPACE] = 1)
		ELSE(cur_pattern > 0, cur_pattern--);
	}

	if (Keys[SC_RIGHT]) {
		ON_PRESS(SC_SPACE, pattern->x < SCREEN_WIDTH/8 - 1,
			pattern->x++; Keys[SC_SPACE] = 1)
		ELSE(cur_pattern < room->pattern_num - 1, cur_pattern++);
	}

	if (Keys[SC_UP]) {
		ON_PRESS(SC_SPACE, pattern->y > 0,
			pattern->y--; Keys[SC_SPACE] = 1);
	}

	if (Keys[SC_DOWN]) {
		ON_PRESS(SC_SPACE, pattern->y < ACTION_SCREEN_HEIGHT/8 - 1,
			pattern->y++; Keys[SC_SPACE] = 1);
	}
}

void DoEdit()
{
	/* Check changing edit mode */
	ON_PRESS(SC_TAB, editmode > 0, editmode--);
	ON_PRESS(SC_BACKSPACE, editmode < EDIT_SPRITESET, editmode++);

	switch (editmode) {
	case EDIT_MAP:		MapEdit(); break;
	case EDIT_ROOMVIEW:	RoomView(); break;
	case EDIT_ROOM:		RoomEdit(); break;
	}
}

