#include <string.h>
#include <stdlib.h>

#include "sdl_compat.h"
#include "world.h"
#include "engine.h"
#include "sprites.h"
#include "room.h"
#include "data.h"

// TODO: create ROOMTILEMAP struct that will hold room dimensions and room_tiles pointer
int GetTileI(unsigned short *room_tiles, int x, int y)
{
    if (x < 0 || x >= ROOM_WIDTH ||
        y < 0 || y >= ROOM_HEIGHT)
        return 0;

    return room_tiles[y * ROOM_WIDTH + x];
}

void SetTileI(unsigned short *room_tiles, int x, int y, int i)
{
    if (x < 0 || x >= ROOM_WIDTH ||
        y < 0 || y >= ROOM_HEIGHT)
        return;

    room_tiles[y * ROOM_WIDTH + x] = i;
}

void UnpackRoom(unsigned short *room_tiles, WORLD *world, int room_idx)
{
    memset(room_tiles, 0, ROOM_WIDTH * ROOM_HEIGHT * sizeof(unsigned short));

    unsigned short *end = room_tiles + ROOM_WIDTH * ROOM_HEIGHT;
    PATTERN *pattern = (world->room + room_idx)->pattern;
    int count = (world->room + room_idx)->pattern_num;

    for (; count-- > 0; pattern++) {
        PATTERNSET *patternset = world->patternset + pattern->index;
        unsigned short *src = patternset->data;
        unsigned short *dst = &room_tiles[pattern->y * ROOM_WIDTH + pattern->x];

        int dy = patternset->ys;
        int dx = patternset->xs;

        for (int y = 0; y < dy; y++, dst += ROOM_WIDTH - dx)
            for (int x = 0; x < dx; x++, src++, dst++) {
                if (dst >= end)
                    break;

                if (x + pattern->x < ROOM_WIDTH && *src)
                    *dst = *src;
            }
    }
}

void BlitRoom(SDL_Surface *surface, unsigned short *room_tiles)
{
    for (int y = 0; y < ROOM_HEIGHT; y++)
        for (int x = 0; x < ROOM_WIDTH; x++)
            PutTileI(surface, x*8, y*8, room_tiles[y*ROOM_WIDTH+x]);
}

void BlitRoomOutlines(SDL_Surface *surface, unsigned short *room_tiles, WORLD *world, int room)
{
    unsigned int shadow = (world->room + room)->shadow;

    for (int y = 0; y < ROOM_HEIGHT; y++)
        for (int x = 0; x < ROOM_WIDTH; x++)
            PutTileS(surface, x*8, y*8, room_tiles[y*ROOM_WIDTH+x], shadow);
}

void BlitBackground(TGAMEDATA *game, SDL_Surface *surface)
{
    SDL_Rect dst = {0, 0, SCREEN_WIDTH, ACTION_SCREEN_HEIGHT};
    SDL_BlitSurface((SDL_Surface *)game->background, NULL, surface, &dst);
}
