#include "engine-coordinates.h"
#include "room.h"
#include "engine.h"
#include "world.h"

int Coords_GetTile(ROOMCONTAINER *rc, int x, int y) {
    return GetTileI(rc->tiles, x, y);
}

void Coords_SetTile(ROOMCONTAINER *rc, int x, int y, int i) {
    SetTileI(rc->tiles, x, y, i);
}

int Coords_GetTileAtWorld(ROOMCONTAINER *rc, int wx, int wy) {
    if (!rc || !rc->tiles) return 1;
    int lx = wx % (ROOM_WIDTH * 8);
    int ly = wy % (ROOM_HEIGHT * 8);
    if (lx < 0) lx += ROOM_WIDTH * 8;
    if (ly < 0) ly += ROOM_HEIGHT * 8;
    return GetTileI(rc->tiles, lx >> 3, ly >> 3);
}

void Coords_SetTileAtWorld(ROOMCONTAINER *rc, int wx, int wy, int i) {
    if (!rc || !rc->tiles) return;
    int lx = wx % (ROOM_WIDTH * 8);
    int ly = wy % (ROOM_HEIGHT * 8);
    if (lx < 0) lx += ROOM_WIDTH * 8;
    if (ly < 0) ly += ROOM_HEIGHT * 8;
    SetTileI(rc->tiles, lx >> 3, ly >> 3, i);
}
