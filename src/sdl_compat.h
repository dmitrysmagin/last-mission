#ifndef SDL_COMPAT_H
#define SDL_COMPAT_H

#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#define SDL_SRCCOLORKEY SDL_TRUE
#define SDL_SWSURFACE 0
#define SDL_HWSURFACE 0
#define SDL_HWPALETTE 0
#define SDL_DOUBLEBUF 0
#define SDL_FULLSCREEN SDL_WINDOW_FULLSCREEN

extern SDL_Window *sdl_window;

SDL_Surface *sdl_set_video_mode(int width, int height, int bpp, Uint32 flags);
int sdl_flip(SDL_Surface *screen);

#define SDL_SetVideoMode sdl_set_video_mode
#define SDL_Flip sdl_flip
#define SDL_WM_SetCaption(title, icon) SDL_SetWindowTitle(sdl_window, title)

#else
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#endif

#endif
