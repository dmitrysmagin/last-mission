#include "engine-collision.h"
#include "engine.h"
#include "engine-scrolling.h"
#include "room.h"
#include "video.h"

#define IS_SCROLLING() (game->engine == &scrolling_engine)

/* Check if gobj1 can destroy gobj2 (or blocks movement) */
static int gObj_CheckDestruction(TSHIP *gobj1, TSHIP *gobj2)
{
    int obj1_type = (gobj1->flags & (GOBJ_PLAYER|GOBJ_WEAPON)) >> 5;
    int obj2_type = (gobj2->flags & (GOBJ_PLAYER|GOBJ_WEAPON)) >> 5;

    if (obj1_type == obj2_type)
        goto _exit_proc;

    if (gobj2->flags & GOBJ_WEAPON && gobj1 == gobj2->parent)
        return 0;

    if (gobj1->flags & GOBJ_WEAPON && gobj2 == gobj1->parent)
        return 0;

    if(gobj2->ai_type == AI_GARAGE)
        goto _exit_proc;

    if (gobj1->ai_type == AI_BONUS) {
        if (gobj2->ai_type != AI_BASE && gobj2->ai_type != AI_SHIP)
            return 0;
        gobj2->regenerate_bonus = 1;
        gobj2->bonus_type = gobj1->bonus_type;
        return 1;
    }

    if (gobj2->flags & GOBJ_DESTROY)
        return 1;

_exit_proc:
    if (!(gobj1->flags & GOBJ_HURTS) || !(gobj2->flags & GOBJ_HURTS))
        return 0;

    if (!(gobj1->flags & GOBJ_SOLID) || !(gobj2->flags & GOBJ_SOLID))
        return 0;

    return 1;
}

int Collision_CheckTile(int x, int y, TSHIP *gobj) {
    int xs, ys;
    int tile_x, tile_y;

    ys = y + gObj_GetHeight(gobj);
    xs = x + gObj_GetWidth(gobj);

    if (IS_SCROLLING()) {
        // Scrolling: world-absolute lookups
        for (tile_y = y; tile_y < ys; tile_y++) {
            for (tile_x = x; tile_x < xs; tile_x++) {
                unsigned char b = Scrolling_GetTileAtWorld(tile_x, tile_y);
                if (b == 0) continue;
                if (gobj->ai_type == AI_SHIP && b >= 246) gObj_Explode(gobj);
                return 1;
            }
        }
    } else {
        // Flip-screen: room-local lookups
        for (tile_y = y; tile_y < ys; tile_y++) {
            for (tile_x = x; tile_x < xs; tile_x++) {
                unsigned char b = GetTileI(game->roomcontainer->tiles, tile_x >> 3, tile_y >> 3);
                if (b == 0) continue;
                if (gobj->ai_type == AI_SHIP && b >= 246) gObj_Explode(gobj);
                return 1;
            }
        }
    }
    return 0;
}

int Collision_CheckOverlap(int x1, int y1, TSHIP *gobj1, int x2, int y2, TSHIP *gobj2) {
    int w1 = gObj_GetWidth(gobj1);
    int h1 = gObj_GetHeight(gobj1);
    int w2 = gObj_GetWidth(gobj2);
    int h2 = gObj_GetHeight(gobj2);
    
    return !(x2 + w2 <= x1 || x2 >= x1 + w1 || y2 + h2 <= y1 || y2 >= y1 + h1);
}

int Collision_CheckSolid(int x, int y, TSHIP *gobj) {
    return Collision_CheckTile(x, y, gobj);
}

int Collision_FlipScreenCheckBounds(int x, int y, int w, int h) {
    if (x < 0 || y < 0)
        return 1;
    if (x + w > SCREEN_WIDTH || y + h > ACTION_SCREEN_HEIGHT)
        return 1;
    return 0;
}

int Collision_ScrollingCheckBounds(int x, int y, int w, int h) {
    // Scrolling allows movement within room boundaries
    // Actual bounds checking is done per-object in gObj_CheckTouch
    (void)x; (void)y; (void)w; (void)h;
    return 0;
}

int Collision_CheckBounds(int x, int y, int w, int h, IGameEngine *engine) {
    if (engine == &scrolling_engine) {
        return Collision_ScrollingCheckBounds(x, y, w, h);
    }
    return Collision_FlipScreenCheckBounds(x, y, w, h);
}

int Collision_CheckObjectBounds(TSHIP *obj, IGameEngine *engine) {
    if (!obj) return 0;
    int w = gObj_GetWidth(obj);
    int h = gObj_GetHeight(obj);
    return Collision_CheckBounds(obj->x, obj->y, w, h, engine);
}

int gObj_CheckTouch(int x, int y, TSHIP *gobj) {
    int xs, ys;
    int result;

    ys = y + gObj_GetHeight(gobj);
    xs = x + gObj_GetWidth(gobj);

    // Check bounds
    if (IS_SCROLLING()) {
        // Scrolling: restrict to native room rect (except elevator)
        if (gobj->home_room != 0 && gobj->ai_type != AI_ELEVATOR) {
            int home_min_x = gobj->home_gx * ROOM_WIDTH * 8;
            int home_max_x = home_min_x + ROOM_WIDTH * 8;
            int home_min_y = gobj->home_gy * ROOM_HEIGHT * 8;
            int home_max_y = home_min_y + ROOM_HEIGHT * 8;
            if (x < home_min_x || y < home_min_y || xs > home_max_x || ys > home_max_y)
                return 1;
        }
    } else {
        // Flip-screen: check screen bounds
        if (Collision_FlipScreenCheckBounds(x, y, xs - x, ys - y))
            return 1;
    }

    // Check object overlap
    GOBJ_ITERATE(en, {
        if (en == gobj) continue;
        if (Collision_CheckOverlap(x, y, gobj, en->x, en->y, en)) {
            if (!gObj_CheckDestruction(gobj, en)) continue;
            return 1;
        }
    });

    // Check tile collision
    result = Collision_CheckTile(x, y, gobj);
    return result;
}
