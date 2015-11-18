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
static int screen = 1;

static void ShowEditInfo()
{
	static char string[32];
	sprintf(string, "ROOM %03i:%03i", screen, game->world->room_num - 1);
	PutString(0*8, 21*8, string);
}

void DoEdit()
{
	if (reinit) {
		EraseBackground(0);
		UnpackLevel(game->world, screen);
		InitGaragesForNewGame();
		GarageRestore();
		gObj_DestroyAll();
		InitEnemies(screen);
		BlitLevel(screen);
		BlitEnemies();
		ShowEditInfo();
		reinit = 0;
	}

	if (Keys[SC_ESCAPE]) {
		SetGameMode(GM_TITLE);
		LM_ResetKeys();
		reinit = 1;
	}

	if (Keys[SC_LEFT]) {
		if (screen > 0) {
			LM_ResetKeys();
			reinit = 1;
			screen--;
		}
	}

	if (Keys[SC_RIGHT]) {
		if (screen < game->world->room_num - 1) {
			LM_ResetKeys();
			reinit = 1;
			screen++;
		}
	}
}

