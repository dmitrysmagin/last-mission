#include "sdl_compat.h"

#ifdef SDL2

SDL_Window *sdl_window = NULL;

SDL_Surface *sdl_set_video_mode(int width, int height, int bpp, Uint32 flags)
{
    (void)bpp;

    Uint32 window_flags = 0;
    if (flags & SDL_WINDOW_FULLSCREEN)
        window_flags |= SDL_WINDOW_FULLSCREEN;

    if (!sdl_window) {
        sdl_window = SDL_CreateWindow("The Last Mission SDL remake",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      width, height, window_flags);
    } else {
        SDL_SetWindowSize(sdl_window, width, height);
        if (flags & SDL_WINDOW_FULLSCREEN)
            SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN);
        else
            SDL_SetWindowFullscreen(sdl_window, 0);
    }

    return SDL_GetWindowSurface(sdl_window);
}

int sdl_flip(SDL_Surface *screen)
{
    (void)screen;
    return SDL_UpdateWindowSurface(sdl_window);
}

#endif
