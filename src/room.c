#include <string.h>
#include <stdlib.h>

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

void BlitRoom(unsigned short *room_tiles)
{
    for (int y = 0; y < ROOM_HEIGHT; y++)
        for (int x = 0; x < ROOM_WIDTH; x++)
            PutTileI(x*8, y*8, room_tiles[y*ROOM_WIDTH+x]);
}

void BlitRoomOutlines(unsigned short *room_tiles, WORLD *world, int room)
{
    unsigned int shadow = (world->room + room)->shadow;

    for (int y = 0; y < ROOM_HEIGHT; y++)
        for (int x = 0; x < ROOM_WIDTH; x++)
            PutTileS(x*8, y*8, room_tiles[y*ROOM_WIDTH+x], shadow);
}

void BlitBackground(WORLD *world, int room_idx)
{
    if (room_idx > world->room_num)
        return;

    EraseBackground((world->room + room_idx)->background);

    for (int i = 0; i < 2; ++i) {
        BGLINE *bgline = (world->room + room_idx)->bgline;
        int count = (world->room + room_idx)->bg_num;
        unsigned int color = (i == 1)
            ? (world->room + room_idx)->line_light
            : (world->room + room_idx)->line_shadow;

        for (int j = 0; j < count; ++j, bgline++) {
            int x1 = bgline->x1;
            int y1 = bgline->y1;
            int x2 = bgline->x2;
            int y2 = bgline->y2;

            if (i == 1) {
                DrawLine(x1, y1, x2, y2, color);
            } else {
                if (x1 == x2)
                    DrawLine(x1 - 1, y1, x2 - 1, y2, color);
                else if (y1 == y2)
                    DrawLine(x1, y1 - 1, x2, y2 - 1, color);
                else
                    DrawLine(x1 - 1, y1, x2 - 1, y2, color);
            }
        }
    }

    if (room_idx > 69 && room_idx < 92) {
        for (int y = 0; y <= 8; y++)
            for (int x = 0; x <= 20; x++)
                PutBgI(x*16 - 4, y*16 - 8, SkyMap[y][x]);
    }
}
