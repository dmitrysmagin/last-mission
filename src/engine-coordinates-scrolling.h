#ifndef _ENGINE_COORDINATES_SCROLLING_H_
#define _ENGINE_COORDINATES_SCROLLING_H_

#include "room.h"
#include "engine-coordinates.h"

typedef struct {
    int x, y;
    int w, h;
} VIEWPORT_S;

VIEWPORT_S *CoordsScroll_GetViewport(void);
ROOMCONTAINER *CoordsScroll_GetContainerForWorld(int wx, int wy);
int CoordsScroll_GetTileAtWorld(int wx, int wy);
void CoordsScroll_SetTileAtWorld(int wx, int wy, int i);

#endif /* _ENGINE_COORDINATES_SCROLLING_H_ */
