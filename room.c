#include <string.h>
#include <stdlib.h>

#include "world.h"
#include "engine.h"
#include "sprites.h"
#include "room.h"
#include "data.h"

#define ROOM_WIDTH (SCREEN_WIDTH/8)
#define ROOM_HEIGHT (ACTION_SCREEN_HEIGHT/8)

static unsigned short screen[ROOM_WIDTH * ROOM_HEIGHT];

int GetTileI(int x, int y)
{
	if (x < 0 || x > ROOM_WIDTH ||
	    y < 0 || y > ROOM_HEIGHT)
		return 0;

	return screen[y * ROOM_WIDTH + x];
}

void SetTileI(int x, int y, int i)
{
	if (x < 0 || x > ROOM_WIDTH ||
	    y < 0 || y > ROOM_HEIGHT)
		return;

	screen[y * ROOM_WIDTH + x] = i;
}

void UnpackLevel(WORLD *world, int room)
{
	memset(screen, 0, sizeof(screen));

	unsigned short *end = screen + sizeof(screen);
	PATTERN *pattern = (world->room + room)->pattern;
	int count = (world->room + room)->pattern_num;

	for (;count-- > 0; pattern++) {
		PATTERNSET *patternset = world->patternset + pattern->index;
		unsigned short *src = patternset->data;
		unsigned short *dst = &screen[pattern->y * ROOM_WIDTH + pattern->x];

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

void BlitLevel(int room)
{
	for (int y = 0; y < ROOM_HEIGHT; y++)
		for (int x = 0; x < ROOM_WIDTH; x++)
			PutTileI(x*8, y*8, screen[y*ROOM_WIDTH+x]);
}

void BlitLevelOutlines(WORLD *world, int room)
{
	unsigned int shadow = (world->room + room)->shadow;

	for (int y = 0; y < ROOM_HEIGHT; y++)
		for (int x = 0; x < ROOM_WIDTH; x++)
			PutTileS(x*8, y*8, screen[y*ROOM_WIDTH+x], shadow);
}

void BlitBackground(WORLD *world, int room)
{
	if (room > world->room_num)
		return;

	EraseBackground((world->room + room)->background);

	for (int i = 0; i < 2; ++i) {
		BGLINE *bgline = (world->room + room)->bgline;
		int count = (world->room + room)->bg_num;
		unsigned int color = (i == 1)
			? (world->room + room)->line_light
			: (world->room + room)->line_shadow;

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

	if (room > 69 && room < 92) {
		for (int y = 0; y <= 8; y++)
			for (int x = 0; x <= 20; x++)
				PutBgI(x*16 - 4, y*16 - 8, SkyMap[y][x]);
	}
}
