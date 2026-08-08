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

/*
 * Note: define __DINGUX__ to build for low resolutions like 320x240
 */
#include <SDL/SDL.h>

#include "video.h"
#include "input.h"
#include "engine.h"

#ifndef __DINGUX__
int scale2x = 1; // 0 - no scale 320x200; 1 - upscale to 640x400
int fullscr = 0; // or SDL_FULLSCREEN
#endif

SDL_Surface *small_screen = NULL;
SDL_Surface *screen = NULL;

int gfx_init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
		return 0;

	atexit(SDL_Quit);

#ifdef __DINGUX__
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
				  GAME_SCREEN_BPP, SDL_SWSURFACE);
#else
	screen = SDL_SetVideoMode(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2,
				  GAME_SCREEN_BPP, SDL_SWSURFACE | fullscr);
#endif
	small_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
					    SCREEN_WIDTH,
					    SCREEN_HEIGHT,
					    GAME_SCREEN_BPP, 0, 0, 0, 0);

	SDL_ShowCursor(SDL_DISABLE);

	//Set the window caption
	SDL_WM_SetCaption("The Last Mission SDL remake", NULL);

	return 1;
}

void gfx_quit()
{
	//SDL_Quit();
}

static void set_scale(int scale)
{
#ifndef __DINGUX__
	scale2x = scale &= 1;

	if (fullscr == SDL_FULLSCREEN)
		scale = 1;

	screen = SDL_SetVideoMode(SCREEN_WIDTH << scale,
				  SCREEN_HEIGHT << scale,
				  GAME_SCREEN_BPP, SDL_SWSURFACE | fullscr);
	input_reset();
#endif
}

void gfx_flip()
{
#ifndef __DINGUX__
	/* toggle sizes x1 or x2 and fullscreen, for win32 and unix only */
	if (Keys[SC_F] == 1) {
		fullscr ^= SDL_FULLSCREEN;
		Keys[SC_F] = 0;
		set_scale(1);
	} else if (Keys[SC_S] == 1 && fullscr == 0) {
		Keys[SC_S] = 0;
		set_scale(scale2x ^ 1);
	}

	if (scale2x) {
#if GAME_SCREEN_BPP == 16
		#define PIXEL Uint16
#elif GAME_SCREEN_BPP == 32
		#define PIXEL Uint32
#else
	#error GAME_SCREEN_BPP could be 16 or 32
#endif

		PIXEL *s = (PIXEL *)small_screen->pixels;
		PIXEL *d = (PIXEL *)screen->pixels;

		for (int y = SCREEN_HEIGHT; y--; d += SCREEN_WIDTH*2)
			for (int x = SCREEN_WIDTH; x--; d += 2, s++) {
				d[0] = *s;
				d[1] = *s;
				d[SCREEN_WIDTH*2] = *s;
				d[SCREEN_WIDTH*2+1] = *s;
			}
	} else
#endif
	{
		SDL_BlitSurface(small_screen, NULL, screen, NULL);
	}

	SDL_Flip(screen);
}

void ClearScreen()
{
	SDL_FillRect(small_screen, NULL, 0);
}