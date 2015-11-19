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

static int reinit = 1;
static int cur_room = 1;
static int cur_patternset = 0;
static int cur_sprite = 0;
static int editmode = 0; /* 0 - view, 1 - edit room */
static int cur_pattern = 0;

static void ShowEditInfo()
{
	static char string[32];

	switch (editmode) {
	case 0: {
		PATTERNSET *patternset = game->world->patternset + cur_patternset;
		unsigned char *data = (unsigned char *)patternset->data;

		for (int y = 0; y < patternset->ys; y++) {
			for (int x = 0; x < patternset->xs; x++) {
				PutTileI(x*8 + 132, y*8 + 22*8, *data++);
			}
		}

		PutSpriteI(30*8, 22*8, cur_sprite, 0);
		}

		sprintf(string, "ROOM    %03i:%03i", cur_room, game->world->room_num - 1);
		PutString(0*8, 21*8, string);
		sprintf(string, "PTRNSET %03i:%03i", cur_patternset, game->world->patternset_num - 1);
		PutString(0*8, 22*8, string);
		sprintf(string, "SPRTSET %03i:%03i", cur_sprite, SPRITE_NUMBER - 1);
		PutString(0*8, 23*8, string);

		PutString(16*8, 20*8, "VIEW MODE");
		break;
	case 1: {
			ROOM *room = game->world->room + cur_room;
			PATTERN *pattern = room->pattern + cur_pattern;
			PATTERNSET *patternset = game->world->patternset + pattern->index;

			PutString(16*8, 20*8, "ROOM EDIT");

			sprintf(string, "PATTERN %03i:%03i", cur_pattern, room->pattern_num - 1);
			PutString(0*8, 21*8, string);

			DrawRect(pattern->x*8,
				 pattern->y*8,
				 patternset->xs * 8 - 1,
				 patternset->ys * 8 - 1, RGB(255, 0, 255));
			break;
		}
	}
}

static void ViewMode()
{
	if (Keys[SC_ENTER]) {
		editmode = 1;
		cur_pattern = 0;
		LM_ResetKeys();
		reinit = 1;
	}

	if (Keys[SC_ESCAPE]) {
		SetGameMode(GM_TITLE);
		LM_ResetKeys();
		reinit = 1;
	}

	if (Keys[SC_Z]) {
		if (cur_patternset > 0) {
			LM_ResetKeys();
			reinit = 1;
			cur_patternset--;
		}
	}

	if (Keys[SC_X]) {
		if (cur_patternset < game->world->patternset_num - 1) {
			LM_ResetKeys();
			reinit = 1;
			cur_patternset++;
		}
	}

	if (Keys[SC_C]) {
		if (cur_sprite > 0) {
			LM_ResetKeys();
			reinit = 1;
			cur_sprite--;
		}
	}

	if (Keys[SC_V]) {
		if (cur_sprite < SPRITE_NUMBER - 1) {
			LM_ResetKeys();
			reinit = 1;
			cur_sprite++;
		}
	}

	if (Keys[SC_LEFT]) {
		if (cur_room > 0) {
			LM_ResetKeys();
			reinit = 1;
			cur_room--;
		}
	}

	if (Keys[SC_RIGHT]) {
		if (cur_room < game->world->room_num - 1) {
			LM_ResetKeys();
			reinit = 1;
			cur_room++;
		}
	}

}

static void RoomPatternEdit()
{
	ROOM *room = game->world->room + cur_room;

	if (Keys[SC_ESCAPE]) {
		editmode = 0;
		LM_ResetKeys();
		reinit = 1;
	}

	if (Keys[SC_LEFT]) {
		if (cur_pattern > 0) {
			LM_ResetKeys();
			reinit = 1;
			cur_pattern--;
		}
	}

	if (Keys[SC_RIGHT]) {
		if (cur_pattern < room->pattern_num - 1) {
			LM_ResetKeys();
			reinit = 1;
			cur_pattern++;
		}
	}
}

void DoEdit()
{
	if (reinit) {
		ClearScreen();
		UnpackLevel(game->world, cur_room);
		InitGaragesForNewGame();
		GarageRestore();
		gObj_DestroyAll();
		InitEnemies(cur_room);
		BlitLevel(cur_room);
		BlitEnemies();
		ShowEditInfo();
		reinit = 0;
	}

	switch (editmode) {
	case 0:
		ViewMode();
		break;
	case 1:
		RoomPatternEdit();
		break;
	}
}

