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
#include <SDL/SDL_rotozoom.h>

#include "video.h"
#include "input.h"
#include "engine.h"

#ifndef __DINGUX__
int scale2x = 1; // 0 - no scale 320x200; 1 - upscale to 640x400
int fullscr = 0; // or SDL_FULLSCREEN
#endif

int LM_GFX_Init();
void LM_GFX_Deinit();
void LM_GFX_SetScale(int scale);

SDL_Surface *small_screen = NULL;
SDL_Surface *screen = NULL;

int LM_Init()
{
	if (LM_GFX_Init() == 0)
		return 0;

	return 1;
}

void LM_Deinit()
{
	LM_GFX_Deinit();
}

int LM_GFX_Init()
{
	//Start SDL
	//if(SDL_Init(SDL_INIT_EVERYTHING) < 0) return 0;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
		return 0;

	atexit(SDL_Quit);

#ifdef __DINGUX__
	screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
#else
	screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE | fullscr);
#endif
	small_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200,
					    GAME_SCREEN_BPP, 0, 0, 0, 0);

	SDL_ShowCursor(SDL_DISABLE);

	//Set the window caption
	SDL_WM_SetCaption("The Last Mission SDL remake", NULL);

	return 1;
}

void LM_GFX_Deinit()
{
	//SDL_Quit();
}

void LM_GFX_Flip()
{
	SDL_Rect dst;

#ifndef __DINGUX__
	/* toggle sizes x1 or x2 and fullscreen, for win32 and unix only */
	if (Keys[SC_F] == 1) {
		fullscr ^= SDL_FULLSCREEN;
		Keys[SC_F] = 0;
		LM_GFX_SetScale(1);
	} else if (Keys[SC_S] == 1 && fullscr == 0) {
		Keys[SC_S] = 0;
		LM_GFX_SetScale(scale2x ^ 1);
	}

	if (scale2x) {
		dst.x = 0;
		dst.y = 40;

		SDL_Surface *tmp = zoomSurface(small_screen, 2, 2, 0);
		SDL_FillRect(screen, NULL, 0);
		SDL_BlitSurface(tmp, NULL, screen, &dst);
		SDL_FreeSurface(tmp);
	} else
#endif
	{
		dst.x = 0;
		dst.y = 20;

		SDL_BlitSurface(small_screen, NULL, screen, &dst);
	}

	SDL_Flip(screen);
}

void LM_GFX_SetScale(int scale)
{
#ifndef __DINGUX__
	scale2x = scale &= 1;

	if (fullscr == SDL_FULLSCREEN)
		scale = 1;

	screen = SDL_SetVideoMode(320 << scale, 240 << scale, 32, SDL_SWSURFACE | fullscr);
	LM_ResetKeys();
#endif
}
