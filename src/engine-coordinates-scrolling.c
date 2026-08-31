#include "engine-coordinates-scrolling.h"
#include "engine.h"
#include "engine-scrolling.h"
#include "room.h"
#include "world.h"
#include "engine-coordinates.h"

VIEWPORT_S *CoordsScroll_GetViewport(void) {
    static VIEWPORT_S viewport;
    Scrolling_GetViewport(&viewport.x, &viewport.y);
    viewport.w = SCREEN_WIDTH;
    viewport.h = ACTION_SCREEN_HEIGHT;
    return &viewport;
}

ROOMCONTAINER *CoordsScroll_GetContainerForWorld(int wx, int wy) {
    return Scrolling_GetContainerForWorld(wx, wy);
}

int CoordsScroll_GetTileAtWorld(int wx, int wy) {
    return Scrolling_GetTileAtWorld(wx, wy);
}

void CoordsScroll_SetTileAtWorld(int wx, int wy, int i) {
    Scrolling_SetTileAtWorld(wx, wy, i);
}
;
