/*
 * Preliminary scrolling engine — map-only, no enemies/HUD/ship.
 * Viewport moves smoothly with arrows; Q from title enters this mode.
 * Retains flip-screen mode untouched.
 */

#include <string.h>
#include <stdlib.h>

#include "sdl_compat.h"
#include "video.h"
#include "input.h"
#include "engine.h"
#include "room.h"
#include "world.h"
#include "sprites.h"
#include "engine-scrolling.h"

#define SCROLL_SPEED 4
#define CACHE_SIZE 4

typedef struct {
    int x, y; // world pixel top-left
    int w, h; // viewport size
} VIEWPORT;

static VIEWPORT scroll_viewport;
static ROOMCONTAINER *scroll_cache[CACHE_SIZE];
static int scroll_cache_inited = 0;
static int scroll_speed = SCROLL_SPEED;

static ROOMCONTAINER *getCacheForRoom(int room_id)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (scroll_cache[i] && scroll_cache[i]->room_id == room_id)
            return scroll_cache[i];
    }
    return NULL;
}

static void ensureCacheForVisible(int x0, int y0, int x1, int y1)
{
    // collect needed room_ids
    int needed[4];
    int needCount = 0;
    for (int gy = y0; gy <= y1; gy++) {
        for (int gx = x0; gx <= x1; gx++) {
            if (gx < 0 || gy < 0 || gx >= game->world->mapw || gy >= game->world->maph)
                continue;
            int room_id = getscreen(gx, gy);
            if (room_id == 0) continue;
            needed[needCount++] = room_id;
        }
    }

    for (int n = 0; n < needCount; n++) {
        int room_id = needed[n];
        if (getCacheForRoom(room_id))
            continue;

        // find slot to reuse: empty or not in needed set
        ROOMCONTAINER *slot = NULL;
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (scroll_cache[i]->room_id == -1) { slot = scroll_cache[i]; break; }
        }
        if (!slot) {
            for (int i = 0; i < CACHE_SIZE; i++) {
                int rid = scroll_cache[i]->room_id;
                int isNeeded = 0;
                for (int k = 0; k < needCount; k++) if (needed[k] == rid) isNeeded = 1;
                if (!isNeeded) { slot = scroll_cache[i]; break; }
            }
        }
        if (!slot) slot = scroll_cache[0]; // fallback LRU

        RoomContainer_Unpack(slot, room_id);
    }
}

static void blitRoomOffset(SDL_Surface *surface, ROOMCONTAINER *rc, int ox, int oy)
{
    unsigned short *tiles = rc->tiles;
    for (int y = 0; y < ROOM_HEIGHT; y++) {
        for (int x = 0; x < ROOM_WIDTH; x++) {
            int tile = tiles[y * ROOM_WIDTH + x];
            if (tile == 0) continue;
            PutTileI(surface, ox + x * 8, oy + y * 8, tile);
        }
    }
}

void Scrolling_Init(void)
{
    if (scroll_cache_inited) Scrolling_Shutdown();

    for (int i = 0; i < CACHE_SIZE; i++) {
        scroll_cache[i] = RoomContainer_Create();
        scroll_cache[i]->room_id = -1;
    }
    scroll_cache_inited = 1;

    int worldW = game->world->mapw * ROOM_WIDTH * 8;
    int worldH = game->world->maph * ROOM_HEIGHT * 8;

    // start at room index == 1
    int start_gx = 0, start_gy = 0;
    int found = 0;
    for (int y = 0; y < game->world->maph && !found; y++) {
        for (int x = 0; x < game->world->mapw && !found; x++) {
            if (getscreen(x, y) == 1) { start_gx = x; start_gy = y; found = 1; }
        }
    }
    // fallback: first non-empty if room 1 not found
    if (!found) {
        for (int y = 0; y < game->world->maph && !found; y++) {
            for (int x = 0; x < game->world->mapw && !found; x++) {
                if (getscreen(x, y) != 0) { start_gx = x; start_gy = y; found = 1; }
            }
        }
    }

    scroll_viewport.w = ROOM_WIDTH * 8;
    scroll_viewport.h = ROOM_HEIGHT * 8;
    scroll_viewport.x = start_gx * ROOM_WIDTH * 8;
    scroll_viewport.y = start_gy * ROOM_HEIGHT * 8;

    // clamp
    if (scroll_viewport.x > worldW - scroll_viewport.w) scroll_viewport.x = worldW - scroll_viewport.w;
    if (scroll_viewport.y > worldH - scroll_viewport.h) scroll_viewport.y = worldH - scroll_viewport.h;
    if (scroll_viewport.x < 0) scroll_viewport.x = 0;
    if (scroll_viewport.y < 0) scroll_viewport.y = 0;

    // prime cache
    int x0 = scroll_viewport.x / (ROOM_WIDTH * 8);
    int y0 = scroll_viewport.y / (ROOM_HEIGHT * 8);
    int x1 = (scroll_viewport.x + scroll_viewport.w - 1) / (ROOM_WIDTH * 8);
    int y1 = (scroll_viewport.y + scroll_viewport.h - 1) / (ROOM_HEIGHT * 8);
    ensureCacheForVisible(x0, y0, x1, y1);
}

void Scrolling_Shutdown(void)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (scroll_cache[i]) {
            RoomContainer_Destroy(scroll_cache[i]);
            scroll_cache[i] = NULL;
        }
    }
    scroll_cache_inited = 0;
}

void DoScrolling(void)
{
    int worldW = game->world->mapw * ROOM_WIDTH * 8;
    int worldH = game->world->maph * ROOM_HEIGHT * 8;

    // input — smooth 4px per frame
    if (Keys[SC_LEFT])  scroll_viewport.x -= scroll_speed;
    if (Keys[SC_RIGHT]) scroll_viewport.x += scroll_speed;
    if (Keys[SC_UP])    scroll_viewport.y -= scroll_speed;
    if (Keys[SC_DOWN])  scroll_viewport.y += scroll_speed;

    if (scroll_viewport.x < 0) scroll_viewport.x = 0;
    if (scroll_viewport.y < 0) scroll_viewport.y = 0;
    if (scroll_viewport.x > worldW - scroll_viewport.w) scroll_viewport.x = worldW - scroll_viewport.w;
    if (scroll_viewport.y > worldH - scroll_viewport.h) scroll_viewport.y = worldH - scroll_viewport.h;

    if (Keys[SC_ESCAPE]) {
        input_reset();
        Scrolling_Shutdown();
        SetGameMode(GM_TITLE);
        return;
    }

    // update cache for current viewport
    int x0 = scroll_viewport.x / (ROOM_WIDTH * 8);
    int y0 = scroll_viewport.y / (ROOM_HEIGHT * 8);
    int x1 = (scroll_viewport.x + scroll_viewport.w - 1) / (ROOM_WIDTH * 8);
    int y1 = (scroll_viewport.y + scroll_viewport.h - 1) / (ROOM_HEIGHT * 8);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= game->world->mapw) x1 = game->world->mapw - 1;
    if (y1 >= game->world->maph) y1 = game->world->maph - 1;
    ensureCacheForVisible(x0, y0, x1, y1);

    // render — map only, no enemies/HUD/ship
    // only ROOM area (ROOM_WIDTH*8 × ROOM_HEIGHT*8 = 320×136) is visible
    SDL_Surface *surface = (SDL_Surface *)game->roomcontainer->surface;
    SDL_Rect viewportRect = {0, 0, ROOM_WIDTH * 8, ROOM_HEIGHT * 8};
    // clear viewport and status area separately
    SDL_FillRect(surface, NULL, 0);
    SDL_SetClipRect(surface, &viewportRect);

    // blit visible rooms: background first, then tiles (to keep correct layering per-room)
    for (int gy = y0; gy <= y1; gy++) {
        for (int gx = x0; gx <= x1; gx++) {
            int room_id = getscreen(gx, gy);
            if (room_id == 0) continue;
            ROOMCONTAINER *rc = getCacheForRoom(room_id);
            if (!rc) continue;
            int dst_x = gx * ROOM_WIDTH * 8 - scroll_viewport.x;
            int dst_y = gy * ROOM_HEIGHT * 8 - scroll_viewport.y;
            SDL_Rect dst = { dst_x, dst_y, ROOM_WIDTH * 8, ROOM_HEIGHT * 8 };
            SDL_BlitSurface((SDL_Surface *)rc->background, NULL, surface, &dst);
        }
    }
    for (int gy = y0; gy <= y1; gy++) {
        for (int gx = x0; gx <= x1; gx++) {
            int room_id = getscreen(gx, gy);
            if (room_id == 0) continue;
            ROOMCONTAINER *rc = getCacheForRoom(room_id);
            if (!rc) continue;
            int dst_x = gx * ROOM_WIDTH * 8 - scroll_viewport.x;
            int dst_y = gy * ROOM_HEIGHT * 8 - scroll_viewport.y;
            blitRoomOffset(surface, rc, dst_x, dst_y);
        }
    }
    SDL_SetClipRect(surface, NULL);

    // debug overlay — viewport world coords (outside clip, still within ROOM area)
    {
        char buf[64];
        sprintf(buf, "SCROLL %d,%d", scroll_viewport.x, scroll_viewport.y);
        PutString(surface, 4, 4, buf);
        PutString(surface, 4, ACTION_SCREEN_HEIGHT - 12, "ARROWS scroll  ESC title");
    }
}
