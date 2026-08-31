#include <stddef.h>
#include "engine-interface.h"
#include "engine.h"
#include "room.h"

static void flipscreen_init(IGameEngine *engine) {
    (void)engine;
}

static void flipscreen_shutdown(IGameEngine *engine) {
    (void)engine;
}

static void flipscreen_update(IGameEngine *engine) {
    (void)engine;
}

static void flipscreen_render(IGameEngine *engine, SDL_Surface *surface) {
    (void)engine;
    extern void BlitRoom(SDL_Surface *, ROOMCONTAINER *);
    extern void BlitRoomOutlines(SDL_Surface *, ROOMCONTAINER *);
    extern void BlitBackground(SDL_Surface *, ROOMCONTAINER *);
    extern void BlitEnemyOutlines(void);
    extern void BlitEnemies(SDL_Surface *);
    extern int modern_background;

    if (modern_background) {
        BlitBackground(surface, game->roomcontainer);
        BlitRoomOutlines(surface, game->roomcontainer);
        BlitEnemyOutlines();
    }
    BlitRoom(surface, game->roomcontainer);
    BlitEnemies(surface);
}

static int flipscreen_get_tile(IGameEngine *engine, int x, int y) {
    (void)engine;
    return GetTileI(game->roomcontainer->tiles, x, y);
}

static void flipscreen_set_tile(IGameEngine *engine, int x, int y, int tile) {
    (void)engine;
    SetTileI(game->roomcontainer->tiles, x, y, tile);
}

static void flipscreen_get_viewport(IGameEngine *engine, int *x, int *y, int *w, int *h) {
    (void)engine;
    if (x) *x = 0;
    if (y) *y = 0;
    if (w) *w = SCREEN_WIDTH;
    if (h) *h = ACTION_SCREEN_HEIGHT;
}

static int flipscreen_tile_to_world(IGameEngine *engine, int tile_x, int tile_y) {
    (void)engine;
    return tile_y * ROOM_WIDTH + tile_x;
}

IGameEngine flipscreen_engine = {
    .init = flipscreen_init,
    .shutdown = flipscreen_shutdown,
    .update = flipscreen_update,
    .render = flipscreen_render,
    .get_tile = flipscreen_get_tile,
    .set_tile = flipscreen_set_tile,
    .get_viewport = flipscreen_get_viewport,
    .tile_to_world = flipscreen_tile_to_world,
    .data = NULL
};
