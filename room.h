#ifndef _ROOM_H_
#define _ROOM_H_

#include "world.h"

int GetTileI(int x, int y);
void SetTileI(int x, int y, int i);
void UnpackLevel(int room);
void BlitLevel(int room);
void BlitLevelOutlines(int room);
void BlitBackground(WORLD *world, int room);

typedef struct {
	unsigned int background;
	unsigned int shadow;
	unsigned int line_light;
	unsigned int line_shadow;
} ScreenDrawInfo;

ScreenDrawInfo *GetScreenDrawInfo(int screen);

#endif //_ROOM_H_
