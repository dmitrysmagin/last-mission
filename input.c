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

#include "video.h"
#include "input.h"
#include "engine.h"

/* game keys */
unsigned char GKeys[7]; // left, right, up, down, fire, pause, quit

/* system keys */
unsigned char Keys[128] = {0};

void LM_ResetKeys()
{
	memset(&Keys[0], 0, 128);
}

int LM_AnyKey()
{
	for (int i = 0; i < 127; i++) {
		if (Keys[i] == 1)
			return 1;
	}

	return 0;
}

char LM_PollEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event) != 0) {
		int key_scan = -1;
		unsigned char key_value = 0;

		if (event.type == SDL_QUIT) {
			Keys[SC_ESCAPE] = 1;
			return 1;
		}

		// unix and dingoo sdl don't have scancodes, so remap usual keys
		if (event.type == SDL_KEYDOWN)
			key_value = 1;
		if (event.type == SDL_KEYUP)
			key_value = 0;

		// Emulate x86 scancodes
		if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
				case SDLK_UP:
					key_scan = SC_UP;
					break;
				case SDLK_DOWN:
					key_scan = SC_DOWN;
					break;
				case SDLK_LEFT:
					key_scan = SC_LEFT;
					break;
				case SDLK_RIGHT:
					key_scan = SC_RIGHT;
					break;
				case SDLK_ESCAPE:
					key_scan = SC_ESCAPE;
					break;
				case SDLK_RETURN: // ENTER
					key_scan = SC_ENTER;
					break;
				case SDLK_LCTRL:
				case SDLK_LALT:
				case SDLK_LSHIFT:
				case SDLK_SPACE:
					key_scan = SC_SPACE;
					break;
				case SDLK_e:
					key_scan = SC_E;
					break;
				case SDLK_c:
					key_scan = SC_C;
					break;
				case SDLK_v:
					key_scan = SC_V;
					break;
				case SDLK_s:
					key_scan = SC_S;
					break;
				case SDLK_f:
					key_scan = SC_F;
					break;
				case SDLK_x:
					key_scan = SC_X;
					break;
				case SDLK_z:
					key_scan = SC_Z;
					break;
				case SDLK_TAB: // LEFT SHOULDER
					key_scan = SC_TAB;
					break;
				case SDLK_BACKSPACE: // RIGHT SHOULDER
					key_scan = SC_BACKSPACE;
					break;
				default:; // maybe use in future
					break;
			}
			if (key_scan != -1)
				Keys[key_scan] = key_value;
		}
	}

	return 0;
}
