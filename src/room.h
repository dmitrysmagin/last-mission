#ifndef _ROOM_H_
#define _ROOM_H_

#include "video.h"
#include "world.h"

#define ROOM_WIDTH (SCREEN_WIDTH/8)
#define ROOM_HEIGHT (ACTION_SCREEN_HEIGHT/8)

int GetTileI(unsigned short *room_tiles, int x, int y);
void SetTileI(unsigned short *room_tiles, int x, int y, int i);
void UnpackRoom(unsigned short *room_tiles, WORLD *world, int room);
void BlitRoom(unsigned short *room_tiles);
void BlitRoomOutlines(unsigned short *room_tiles, WORLD *world, int room);
void BlitBackground(WORLD *world, int room);

#endif //_ROOM_H_
