#ifndef _ROOM_H_
#define _ROOM_H_

#include "world.h"

int GetTileI(int x, int y);
void SetTileI(int x, int y, int i);
void UnpackRoom(WORLD *world, int room);
void BlitRoom();
void BlitRoomOutlines(WORLD *world, int room);
void BlitBackground(WORLD *world, int room);

#endif //_ROOM_H_
