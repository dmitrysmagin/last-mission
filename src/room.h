#ifndef _ROOM_H_
#define _ROOM_H_

#include "video.h"
#include "world.h"

typedef struct SDL_Surface SDL_Surface;
typedef struct TGAMEDATA TGAMEDATA;

#define ROOM_WIDTH (SCREEN_WIDTH/8)
#define ROOM_HEIGHT (ACTION_SCREEN_HEIGHT/8)

typedef struct ROOMCONTAINER {
    unsigned short *tiles; /* ROOM_WIDTH * ROOM_HEIGHT */
    void *background; /* pre-rendered background pixels, SCREEN_WIDTH x ACTION_SCREEN_HEIGHT */
    void *surface; /* room rendering surface, SCREEN_WIDTH x SCREEN_HEIGHT */
    unsigned int shadow; /* room shadow color for outlines */
    int room_id;
} ROOMCONTAINER;

ROOMCONTAINER *RoomContainer_Create(void);
void RoomContainer_Destroy(ROOMCONTAINER *rc);
void RoomContainer_Unpack(ROOMCONTAINER *rc, int room);

int GetTileI(unsigned short *room_tiles, int x, int y);
void SetTileI(unsigned short *room_tiles, int x, int y, int i);
void UnpackRoom(ROOMCONTAINER *roomcontainer, int room);
void BlitRoom(SDL_Surface *surface, ROOMCONTAINER *roomcontainer);
void BlitRoomOutlines(SDL_Surface *surface, ROOMCONTAINER *roomcontainer);
void BlitBackground(SDL_Surface *surface, ROOMCONTAINER *roomcontainer);

#endif //_ROOM_H_
