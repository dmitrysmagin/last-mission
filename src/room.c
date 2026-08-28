#include <string.h>
#include <stdlib.h>

#include "sdl_compat.h"
#include "world.h"
#include "engine.h"
#include "sprites.h"
#include "room.h"
#include "data.h"

ROOMCONTAINER *RoomContainer_Create(void)
{
    ROOMCONTAINER *rc = (ROOMCONTAINER *)malloc(sizeof(ROOMCONTAINER));
    if (!rc) return NULL;
    rc->tiles = (unsigned short *)calloc(ROOM_WIDTH * ROOM_HEIGHT, sizeof(unsigned short));
    rc->background = SDL_CreateRGBSurface(SDL_SWSURFACE,
        SCREEN_WIDTH, ACTION_SCREEN_HEIGHT,
        GAME_SCREEN_BPP, 0, 0, 0, 0);
    rc->surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        GAME_SCREEN_BPP, 0, 0, 0, 0);
    rc->shadow = 0;
    rc->room_id = -1;
    return rc;
}

void RoomContainer_Destroy(ROOMCONTAINER *rc)
{
    if (!rc) return;
    if (rc->tiles) free(rc->tiles);
    if (rc->background) SDL_FreeSurface((SDL_Surface *)rc->background);
    if (rc->surface) SDL_FreeSurface((SDL_Surface *)rc->surface);
    free(rc);
}

void RoomContainer_Unpack(ROOMCONTAINER *rc, int room)
{
    if (!rc) return;
    UnpackRoom(rc, room);
}

int GetTileI(unsigned short *tiles, int x, int y)
{
    if (x < 0 || x >= ROOM_WIDTH ||
        y < 0 || y >= ROOM_HEIGHT)
        return 0;

    return tiles[y * ROOM_WIDTH + x];
}

void SetTileI(unsigned short *tiles, int x, int y, int i)
{
    if (x < 0 || x >= ROOM_WIDTH ||
        y < 0 || y >= ROOM_HEIGHT)
        return;

    tiles[y * ROOM_WIDTH + x] = i;
}

static void ClipLine(int *x1, int *y1, int *x2, int *y2)
{
    if (*x1 < 0) *x1 = 0;
    if (*x2 < 0) *x2 = 0;
    if (*x1 >= SCREEN_WIDTH) *x1 = SCREEN_WIDTH - 1;
    if (*x2 >= SCREEN_WIDTH) *x2 = SCREEN_WIDTH - 1;
    if (*y1 < 0) *y1 = 0;
    if (*y2 < 0) *y2 = 0;
    if (*y1 >= ACTION_SCREEN_HEIGHT) *y1 = ACTION_SCREEN_HEIGHT - 1;
    if (*y2 >= ACTION_SCREEN_HEIGHT) *y2 = ACTION_SCREEN_HEIGHT - 1;
}

static void BuildBackgroundForContainer(ROOMCONTAINER *roomcontainer, int room_idx)
{
    WORLD *world = game->world;
    ROOM *room = world->room + room_idx;
    SDL_Surface *bg = (SDL_Surface *)roomcontainer->background;

    EraseBackground(bg, room->background);

    for (int i = 0; i < 2; ++i) {
        BGLINE *bgline = room->bgline;
        int count = room->bg_num;
        unsigned int color = (i == 1)
            ? room->line_light
            : room->line_shadow;

        for (int j = 0; j < count; ++j, bgline++) {
            int x1 = bgline->x1;
            int y1 = bgline->y1;
            int x2 = bgline->x2;
            int y2 = bgline->y2;

            if (i == 0) {
                if (x1 == x2) { x1--; x2--; }
                else if (y1 == y2) { y1--; y2--; }
                else { x1--; x2--; }
            }

            ClipLine(&x1, &y1, &x2, &y2);
            DrawLine(bg, x1, y1, x2, y2, color);
        }
    }

    if (room_idx > 69 && room_idx < 92) {
        for (int y = 0; y <= 8; y++)
            for (int x = 0; x <= 20; x++) {
                int sx = x * 16 - 4;
                int sy = y * 16 - 8;

                if (sx >= SCREEN_WIDTH || sy >= ACTION_SCREEN_HEIGHT)
                    continue;

                PutBgI(bg, sx, sy, SkyMap[y][x]);
            }
    }
}

void UnpackRoom(ROOMCONTAINER *roomcontainer, int room_idx)
{
    WORLD *world = game->world;
    unsigned short *room_tiles = roomcontainer->tiles;
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
    roomcontainer->shadow = (world->room + room_idx)->shadow;
    roomcontainer->room_id = room_idx;

    BuildBackgroundForContainer(roomcontainer, room_idx);
}

void BlitRoom(SDL_Surface *surface, ROOMCONTAINER *roomcontainer)
{
    unsigned short *room_tiles = roomcontainer->tiles;
    for (int y = 0; y < ROOM_HEIGHT; y++)
        for (int x = 0; x < ROOM_WIDTH; x++)
            PutTileI(surface, x*8, y*8, room_tiles[y*ROOM_WIDTH+x]);
}

void BlitRoomOutlines(SDL_Surface *surface, ROOMCONTAINER *roomcontainer)
{
    unsigned int shadow = roomcontainer->shadow;
    unsigned short *room_tiles = roomcontainer->tiles;

    for (int y = 0; y < ROOM_HEIGHT; y++)
        for (int x = 0; x < ROOM_WIDTH; x++)
            PutTileS(surface, x*8, y*8, room_tiles[y*ROOM_WIDTH+x], shadow);
}

void BlitBackground(SDL_Surface *surface, ROOMCONTAINER *roomcontainer)
{
    SDL_Rect dst = {0, 0, SCREEN_WIDTH, ACTION_SCREEN_HEIGHT};
    SDL_BlitSurface((SDL_Surface *)roomcontainer->background, NULL, surface, &dst);
}
