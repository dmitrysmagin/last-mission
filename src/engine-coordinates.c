#include <stddef.h>
#include "engine-coordinates.h"
#include "engine.h"
#include "room.h"
#include "video.h"

int Coords_WorldToScreenX(int worldX, IGameEngine *engine) {
    int vx, vy;
    if (engine && engine->get_viewport) {
        engine->get_viewport(engine, &vx, &vy, NULL, NULL);
        return worldX - vx;
    }
    return worldX;
}

int Coords_WorldToScreenY(int worldY, IGameEngine *engine) {
    int vx, vy;
    if (engine && engine->get_viewport) {
        engine->get_viewport(engine, &vx, &vy, NULL, NULL);
        return worldY - vy;
    }
    return worldY;
}

int Coords_ScreenToWorldX(int screenX, IGameEngine *engine) {
    int vx, vy;
    if (engine && engine->get_viewport) {
        engine->get_viewport(engine, &vx, &vy, NULL, NULL);
        return screenX + vx;
    }
    return screenX;
}

int Coords_ScreenToWorldY(int screenY, IGameEngine *engine) {
    int vx, vy;
    if (engine && engine->get_viewport) {
        engine->get_viewport(engine, &vx, &vy, NULL, NULL);
        return screenY + vy;
    }
    return screenY;
}

int Coords_ObjectGetScreenX(TSHIP *obj, IGameEngine *engine) {
    if (!obj) return 0;

    // For scrolling mode, objects store world coordinates
    // For flip-screen, objects store screen coordinates directly
    if (game->mode == GM_SCROLLING) {
        return Coords_WorldToScreenX(obj->x, engine);
    }
    return obj->x;
}

int Coords_ObjectGetScreenY(TSHIP *obj, IGameEngine *engine) {
    if (!obj) return 0;

    if (game->mode == GM_SCROLLING) {
        return Coords_WorldToScreenY(obj->y, engine);
    }
    return obj->y;
}

int Coords_ObjectInViewport(TSHIP *obj, IGameEngine *engine) {
    if (!obj || !engine) return 0;

    int sx = Coords_ObjectGetScreenX(obj, engine);
    int sy = Coords_ObjectGetScreenY(obj, engine);
    int w = gObj_GetWidth(obj);
    int h = gObj_GetHeight(obj);

    return (sx + w > 0 && sx < SCREEN_WIDTH &&
            sy + h > 0 && sy < ACTION_SCREEN_HEIGHT);
}

int Coords_GetRoomWidth(void) {
    return ROOM_WIDTH;
}

int Coords_GetRoomHeight(void) {
    return ROOM_HEIGHT;
}

int Coords_WorldToTileX(int worldX) {
    return worldX >> 3;
}

int Coords_WorldToTileY(int worldY) {
    return worldY >> 3;
}
