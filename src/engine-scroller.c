#include <stddef.h>
#include "engine-interface.h"
#include "engine.h"
#include "room.h"
#include "engine-scrolling.h"

static void scroller_init(IGameEngine *engine) {
    (void)engine;
    Scrolling_Init();
}

static void scroller_shutdown(IGameEngine *engine) {
    (void)engine;
    Scrolling_Shutdown();
}

static void scroller_update(IGameEngine *engine) {
    (void)engine;
    DoScrolling();
}

static void scroller_render(IGameEngine *engine, SDL_Surface *surface) {
    (void)engine;
    (void)surface;
}

static int scroller_get_tile(IGameEngine *engine, int x, int y) {
    (void)engine;
    int wx = x * 8;
    int wy = y * 8;
    return Scrolling_GetTileAtWorld(wx, wy);
}

static void scroller_set_tile(IGameEngine *engine, int x, int y, int tile) {
    (void)engine;
    int wx = x * 8;
    int wy = y * 8;
    Scrolling_SetTileAtWorld(wx, wy, tile);
}

static void scroller_get_viewport(IGameEngine *engine, int *x, int *y, int *w, int *h) {
    (void)engine;
    Scrolling_GetViewport(x, y);
    if (w) *w = SCREEN_WIDTH;
    if (h) *h = ACTION_SCREEN_HEIGHT;
}

static int scroller_tile_to_world(IGameEngine *engine, int tile_x, int tile_y) {
    (void)engine;
    return tile_y * (game->world->mapw * ROOM_WIDTH) + tile_x;
}

IGameEngine scrolling_engine = {
    .init = scroller_init,
    .shutdown = scroller_shutdown,
    .update = scroller_update,
    .render = scroller_render,
    .get_tile = scroller_get_tile,
    .set_tile = scroller_set_tile,
    .get_viewport = scroller_get_viewport,
    .tile_to_world = scroller_tile_to_world,
    .data = NULL
};
