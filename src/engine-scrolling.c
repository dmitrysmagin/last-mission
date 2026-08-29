/*
 * Preliminary scrolling engine — map + enemies, no HUD/ship.
 * Viewport moves smoothly with arrows; Q from title enters this mode.
 * Enemies are world-absolute, global pool, activated when viewport hits native room,
 * restricted to native room rectangle.
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
#include "object.h"
#include "object_garage.h"
#include "engine-scrolling.h"

#define SCROLL_SPEED 4
#define CACHE_SIZE 4
#define MAX_ROOM_ID 256

typedef struct {
    int x, y; // world pixel top-left
    int w, h; // viewport size
} VIEWPORT;

static VIEWPORT scroll_viewport;
static ROOMCONTAINER *scroll_cache[CACHE_SIZE];
static int scroll_cache_inited = 0;
static int spawnedRoom[MAX_ROOM_ID];

static ROOMCONTAINER *getCacheForRoom(int room_id)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (scroll_cache[i] && scroll_cache[i]->room_id == room_id)
            return scroll_cache[i];
    }
    return NULL;
}

ROOMCONTAINER *Scrolling_GetContainerForWorld(int wx, int wy)
{
    int gx = wx / (ROOM_WIDTH * 8);
    int gy = wy / (ROOM_HEIGHT * 8);
    if (gx < 0 || gy < 0 || gx >= game->world->mapw || gy >= game->world->maph)
        return NULL;
    int room_id = getscreen(gx, gy);
    if (room_id == 0) return NULL;
    return getCacheForRoom(room_id);
}

int Scrolling_GetTileAtWorld(int wx, int wy)
{
    ROOMCONTAINER *rc = Scrolling_GetContainerForWorld(wx, wy);
    if (!rc || !rc->tiles) return 1; // treat missing as solid
    int lx = wx % (ROOM_WIDTH * 8);
    int ly = wy % (ROOM_HEIGHT * 8);
    if (lx < 0) lx += ROOM_WIDTH * 8;
    if (ly < 0) ly += ROOM_HEIGHT * 8;
    return GetTileI(rc->tiles, lx >> 3, ly >> 3);
}

void Scrolling_SetTileAtWorld(int wx, int wy, int tile)
{
    ROOMCONTAINER *rc = Scrolling_GetContainerForWorld(wx, wy);
    if (!rc || !rc->tiles) return;
    int lx = wx % (ROOM_WIDTH * 8);
    int ly = wy % (ROOM_HEIGHT * 8);
    if (lx < 0) lx += ROOM_WIDTH * 8;
    if (ly < 0) ly += ROOM_HEIGHT * 8;
    SetTileI(rc->tiles, lx >> 3, ly >> 3, tile);
}

static void scrollDespawnRoom(int room_id)
{
    if (room_id < 0 || room_id >= MAX_ROOM_ID) return;
    if (!spawnedRoom[room_id]) return;
    GOBJ_ITERATE_SAFE(gobj, {
        if (gobj->home_room == room_id) gObj_DestroyObject(gobj);
    });
    spawnedRoom[room_id] = 0;
}

static void ensureCacheForVisible(int x0, int y0, int x1, int y1)
{
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
        if (!slot) slot = scroll_cache[0];
        // evict old room's enemies before reuse
        if (slot->room_id != -1) scrollDespawnRoom(slot->room_id);
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

static void scrollSpawnRoom(int room_id, int gx, int gy)
{
    if (room_id == 0 || room_id >= MAX_ROOM_ID) return;
    if (spawnedRoom[room_id]) return;
    spawnedRoom[room_id] = 1;

    ROOM *room = game->world->room + room_id;
    OBJECT *object = room->object;
    int count = room->object_num;

    for (; count > 0; count--, object++) {
        if (object->index == BONUS_FACEBOOK || object->index == BONUS_TWITTER) {
            if (game->mode == GM_DEMO) continue;
        }
        // skip garage hidden? keep similar to InitEnemies
        TSHIP *en = gObj_CreateObject();
        if (!en) break; // pool exhausted — guard hijack
        en->i = object->index;
        en->x = gx * ROOM_WIDTH * 8 + object->x;
        en->y = gy * ROOM_HEIGHT * 8 + object->y;
        en->world_x = en->x;
        en->world_y = en->y;
        en->home_room = room_id;
        en->home_gx = gx;
        en->home_gy = gy;
        en->scroll_active = 0;
        en->anim_speed = object->speed;
        en->anim_speed_cnt = object->speed;
        en->min_frame = object->minframe;
        en->cur_frame = object->minframe;
        en->max_frame = object->maxframe;
        gObj_Constructor(en, object->ai);
        en->move_speed = 1;
        en->move_speed_cnt = 1;

        if (en->i == 11 || en->i == 42)
            en->flags &= ~GOBJ_DESTROY;
        if (en->ai_type == AI_HOMING_MISSLE && !game->easy_mode) {
            en->flags &= ~(GOBJ_SOLID|GOBJ_DESTROY);
        }
        if (en->ai_type == AI_GARAGE) {
            CreateGarage(en, object->garage_id);
        } else if (en->ai_type == AI_HIDDEN_AREA_ACCESS) {
            en->dx = object->speed;
            en->dy = object->minframe;
        }
        // garages and hidden access already handle gx offset via world_x
    }
}

static void scrollEnsureEnemiesForVisible(int x0, int y0, int x1, int y1)
{
    for (int gy = y0; gy <= y1; gy++) {
        for (int gx = x0; gx <= x1; gx++) {
            if (gx < 0 || gy < 0 || gx >= game->world->mapw || gy >= game->world->maph) continue;
            int room_id = getscreen(gx, gy);
            if (room_id == 0) continue;
            scrollSpawnRoom(room_id, gx, gy);
        }
    }
}

static void scrollSpawnShip(int gx, int gy)
{
    // replicate InitShip but world-absolute
    TSHIP *base = gObj_CreateObject();
    if (!base) return;
    TSHIP *ship = gObj_CreateObject();
    if (!ship) { gObj_DestroyObject(base); return; }

    int base_world_x = gx * ROOM_WIDTH * 8 + 148;
    int base_world_y = gy * ROOM_HEIGHT * 8 + 104;

    base->x = base_world_x;
    base->y = base_world_y;
    base->world_x = base->x;
    base->world_y = base->y;
    base->home_room = getscreen(gx, gy);
    base->home_gx = gx;
    base->home_gy = gy;
    base->i = 1;
    base->min_frame = 0;
    base->cur_frame = 0;
    base->max_frame = 1;
    base->anim_speed = 0;
    base->anim_speed_cnt = 0;
    base->parent = ship;
    gObj_Constructor(base, AI_BASE);

    int ship_idx = 0; // default laser ship; could use GetPlayerShipIndex()
    // try to use player's chosen ship if available
    extern int GetPlayerShipIndex(void);
    ship_idx = GetPlayerShipIndex();

    ship->i = ship_idx;
    // offset to center on base: (40 - shipW)/2
    int shipW = GetSpriteW(ship_idx);
    int offset = (40 - shipW) / 2;
    ship->x = base_world_x + offset;
    ship->y = base_world_y - 36; // 68 = 104-36 as in InitShip
    ship->world_x = ship->x;
    ship->world_y = ship->y;
    // ship free to roam whole world (not restricted to native room)
    ship->home_room = 0;
    ship->home_gx = 0;
    ship->home_gy = 0;
    ship->base = base;
    ship->smoke = NULL;
    ship->laser = NULL;
    ship->scroll_active = 1;

    if (ship_idx == SHIP_TYPE_BFG) { ship->min_frame = 0; ship->max_frame = 4; }
    else if (ship_idx == SHIP_TYPE_OBSERVER) { ship->min_frame = 1; ship->max_frame = 3; }
    else { ship->min_frame = 0; ship->max_frame = 6; }

    ship->cur_frame = 0;
    ship->anim_speed = 1;
    ship->anim_speed_cnt = 1;
    gObj_Constructor(ship, AI_SHIP);

    base->scroll_active = 1;
    // base also free (follows ship when attached, otherwise static)
    base->home_room = 0;
    base->home_gx = 0;
    base->home_gy = 0;
}

static int rectIntersect(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return !(ax + aw <= bx || bx + bw <= ax || ay + ah <= by || by + bh <= ay);
}

void Scrolling_Init(void)
{
    if (scroll_cache_inited) Scrolling_Shutdown();

    for (int i = 0; i < CACHE_SIZE; i++) {
        scroll_cache[i] = RoomContainer_Create();
        scroll_cache[i]->room_id = -1;
    }
    scroll_cache_inited = 1;
    memset(spawnedRoom, 0, sizeof(spawnedRoom));

    // clear enemy pool (keep ship/base not needed for scrolling preview)
    gObj_DestroyAll();

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

    if (scroll_viewport.x > worldW - scroll_viewport.w) scroll_viewport.x = worldW - scroll_viewport.w;
    if (scroll_viewport.y > worldH - scroll_viewport.h) scroll_viewport.y = worldH - scroll_viewport.h;
    if (scroll_viewport.x < 0) scroll_viewport.x = 0;
    if (scroll_viewport.y < 0) scroll_viewport.y = 0;

    int x0 = scroll_viewport.x / (ROOM_WIDTH * 8);
    int y0 = scroll_viewport.y / (ROOM_HEIGHT * 8);
    int x1 = (scroll_viewport.x + scroll_viewport.w - 1) / (ROOM_WIDTH * 8);
    int y1 = (scroll_viewport.y + scroll_viewport.h - 1) / (ROOM_HEIGHT * 8);
    ensureCacheForVisible(x0, y0, x1, y1);
    scrollEnsureEnemiesForVisible(x0, y0, x1, y1);
    scrollSpawnShip(start_gx, start_gy);
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
    memset(spawnedRoom, 0, sizeof(spawnedRoom));
    // clear scrolling enemies, restore flip-screen state will re-init via InitNewScreen when needed
    gObj_DestroyAll();
}

void DoScrolling(void)
{
    int worldW = game->world->mapw * ROOM_WIDTH * 8;
    int worldH = game->world->maph * ROOM_HEIGHT * 8;

    if (Keys[SC_ESCAPE]) {
        input_reset();
        Scrolling_Shutdown();
        SetGameMode(GM_TITLE);
        return;
    }

    // map input to game keys for ship
    GKeys[KEY_LEFT]  = Keys[SC_LEFT];
    GKeys[KEY_RIGHT] = Keys[SC_RIGHT];
    GKeys[KEY_UP]    = Keys[SC_UP];
    GKeys[KEY_DOWN]  = Keys[SC_DOWN];
    GKeys[KEY_FIRE]  = Keys[SC_SPACE];
    GKeys[KEY_PAUSE] = 0;
    GKeys[KEY_QUIT]  = 0;

    // update ship/base — world-absolute (Update_Ship has GM_SCROLLING branch)
    TSHIP *ship = gObj_Ship();
    if (ship) {
        TSHIP *base = ship->base;
        gObj_Update(ship);
        if (base && base->state != SH_DEAD) gObj_Update(base);
        // center viewport on ship
        int sx = ship->x + gObj_GetWidth(ship)/2;
        int sy = ship->y + gObj_GetHeight(ship)/2;
        scroll_viewport.x = sx - scroll_viewport.w/2;
        scroll_viewport.y = sy - scroll_viewport.h/2;
        if (scroll_viewport.x < 0) scroll_viewport.x = 0;
        if (scroll_viewport.y < 0) scroll_viewport.y = 0;
        if (scroll_viewport.x > worldW - scroll_viewport.w) scroll_viewport.x = worldW - scroll_viewport.w;
        if (scroll_viewport.y > worldH - scroll_viewport.h) scroll_viewport.y = worldH - scroll_viewport.h;
    } else {
        // fallback: direct viewport nudge if no ship
        if (Keys[SC_LEFT])  scroll_viewport.x -= 4;
        if (Keys[SC_RIGHT]) scroll_viewport.x += 4;
        if (Keys[SC_UP])    scroll_viewport.y -= 4;
        if (Keys[SC_DOWN])  scroll_viewport.y += 4;
        if (scroll_viewport.x < 0) scroll_viewport.x = 0;
        if (scroll_viewport.y < 0) scroll_viewport.y = 0;
        if (scroll_viewport.x > worldW - scroll_viewport.w) scroll_viewport.x = worldW - scroll_viewport.w;
        if (scroll_viewport.y > worldH - scroll_viewport.h) scroll_viewport.y = worldH - scroll_viewport.h;
    }

    int x0 = scroll_viewport.x / (ROOM_WIDTH * 8);
    int y0 = scroll_viewport.y / (ROOM_HEIGHT * 8);
    int x1 = (scroll_viewport.x + scroll_viewport.w - 1) / (ROOM_WIDTH * 8);
    int y1 = (scroll_viewport.y + scroll_viewport.h - 1) / (ROOM_WIDTH * 8);
    y1 = (scroll_viewport.y + scroll_viewport.h - 1) / (ROOM_HEIGHT * 8);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= game->world->mapw) x1 = game->world->mapw - 1;
    if (y1 >= game->world->maph) y1 = game->world->maph - 1;
    ensureCacheForVisible(x0, y0, x1, y1);
    scrollEnsureEnemiesForVisible(x0, y0, x1, y1);

    // update enemies that are in viewport (activate) — world-absolute
    GOBJ_ITERATE(gobj, {
        if (gobj->ai_type == AI_SHIP || gobj->ai_type == AI_BASE) continue; // ship/base already updated
        if (gobj->home_room == 0) {
            gObj_Update(gobj);
            continue;
        }
        int w = gObj_GetWidth(gobj);
        int h = gObj_GetHeight(gobj);
        int intersects = rectIntersect(gobj->x, gobj->y, w, h,
                                       scroll_viewport.x, scroll_viewport.y,
                                       scroll_viewport.w, scroll_viewport.h);
        if (intersects) gobj->scroll_active = 1;
        if (gobj->scroll_active) {
            gobj->world_x = gobj->x;
            gobj->world_y = gobj->y;
            int home_min_x = gobj->home_gx * ROOM_WIDTH * 8;
            int home_max_x = home_min_x + ROOM_WIDTH * 8;
            int home_min_y = gobj->home_gy * ROOM_HEIGHT * 8;
            int home_max_y = home_min_y + ROOM_HEIGHT * 8;
            int old_x = gobj->x, old_y = gobj->y;
            gObj_Update(gobj);
            if (gobj->x < home_min_x) gobj->x = home_min_x;
            if (gobj->y < home_min_y) gobj->y = home_min_y;
            if (gobj->x + w > home_max_x) gobj->x = home_max_x - w;
            if (gobj->y + h > home_max_y) gobj->y = home_max_y - h;
            gobj->world_x = gobj->x;
            gobj->world_y = gobj->y;
            (void)old_x; (void)old_y;
        } else {
            UpdateAnimation(gobj);
        }
    });

    // render — map only, no HUD/ship
    SDL_Surface *surface = (SDL_Surface *)game->roomcontainer->surface;
    SDL_Rect viewportRect = {0, 0, ROOM_WIDTH * 8, ROOM_HEIGHT * 8};
    SDL_FillRect(surface, NULL, 0);
    SDL_SetClipRect(surface, &viewportRect);

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
    // render enemies + weapons with viewport offset
    GOBJ_ITERATE(gobj, {
        if (gobj->ai_type == AI_SHIP || gobj->ai_type == AI_BASE) continue;
        int w = gObj_GetWidth(gobj);
        int h = gObj_GetHeight(gobj);
        if (!rectIntersect(gobj->x, gobj->y, w, h, scroll_viewport.x, scroll_viewport.y, scroll_viewport.w, scroll_viewport.h))
            continue;
        int sx = gobj->x - scroll_viewport.x;
        int sy = gobj->y - scroll_viewport.y;
        if (gobj->ai_type == AI_LASER) {
            DrawLine(surface, sx, sy, sx + gobj->dx, sy, RGB(170,170,170));
        } else if (gobj->flags & GOBJ_VISIBLE) {
            PutSpriteI(surface, sx, sy, gobj->i, gobj->cur_frame);
        }
    });
    // render ship and base (world-absolute, viewport offset, no home restriction)
    {
        TSHIP *ship2 = gObj_Ship();
        if (ship2) {
            int w = gObj_GetWidth(ship2);
            int h = gObj_GetHeight(ship2);
            if (rectIntersect(ship2->x, ship2->y, w, h, scroll_viewport.x, scroll_viewport.y, scroll_viewport.w, scroll_viewport.h)) {
                int sx = ship2->x - scroll_viewport.x;
                int sy = ship2->y - scroll_viewport.y;
                if (ship2->flags & GOBJ_VISIBLE)
                    PutSpriteI(surface, sx, sy, ship2->i, ship2->cur_frame);
            }
            TSHIP *base = ship2->base;
            if (base && base->state == SH_ACTIVE) {
                int bw = gObj_GetWidth(base);
                int bh = gObj_GetHeight(base);
                if (rectIntersect(base->x, base->y, bw, bh, scroll_viewport.x, scroll_viewport.y, scroll_viewport.w, scroll_viewport.h)) {
                    int sx = base->x - scroll_viewport.x;
                    int sy = base->y - scroll_viewport.y;
                    if (base->flags & GOBJ_VISIBLE)
                        PutSpriteI(surface, sx, sy, base->i, base->cur_frame);
                }
            }
        }
    }
    SDL_SetClipRect(surface, NULL);

    // HUD — same as flip-screen (status bar at STATUS_YPOS, no clipping)
    {
        extern void BlitStatus(void);
        extern void BlitStatusData(void);
        BlitStatus();
        BlitStatusData();
    }

    {
        char buf[64];
        sprintf(buf, "SCROLL %d,%d", scroll_viewport.x, scroll_viewport.y);
        PutString(surface, 4, 4, buf);
        // keep debug hint above HUD
        PutString(surface, 4, STATUS_YPOS - 12, "ARROWS move  SPACE fire  ESC title");
    }
}
