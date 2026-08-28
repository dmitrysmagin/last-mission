#ifndef _ROOM_H_
#define _ROOM_H_

#include "video.h"
#include "world.h"

typedef struct SDL_Surface SDL_Surface;
typedef struct TGAMEDATA TGAMEDATA;

#define ROOM_WIDTH (SCREEN_WIDTH/8)
#define ROOM_HEIGHT (ACTION_SCREEN_HEIGHT/8)

int GetTileI(unsigned short *room_tiles, int x, int y);
void SetTileI(unsigned short *room_tiles, int x, int y, int i);
void UnpackRoom(unsigned short *room_tiles, WORLD *world, int room);
void BlitRoom(SDL_Surface *surface, unsigned short *room_tiles);
void BlitRoomOutlines(SDL_Surface *surface, unsigned short *room_tiles, WORLD *world, int room);
void BlitBackground(TGAMEDATA *game, SDL_Surface *surface);

#endif //_ROOM_H_
