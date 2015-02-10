
#include <string.h>
#include <stdlib.h>

#include "sprites.h"
#include "m_aux.h"
#include "m_data.h"

// external data in m_core.c
extern unsigned char ScreenTilesBuffer[0x2a8];

#define NumBackgrounds 13

static ScreenDrawInfo data[NumBackgrounds] = {
	{ RGB( 56, 56, 56), RGB( 32, 32, 32), RGB( 44, 64, 64), RGB( 44, 52, 64) },
	{ RGB(113,  0,  0), RGB( 64,  0,  0), RGB(113, 28,  0), RGB(  0,  0,  0) },
	{ RGB( 56, 56, 13), RGB( 32, 32, 64), RGB( 56, 68,113), RGB( 44, 52, 64) },
	{ RGB(113, 89, 80), RGB( 64, 48, 44), RGB(113,105, 80), RGB(113, 80, 80) },
	{ RGB(113, 56,  0), RGB( 64, 32,  0), RGB(113, 85,  0), RGB(113,  0, 28) },
	{ RGB(  0, 56, 13), RGB(  0, 32, 64), RGB( 56, 85,113), RGB( 44, 52, 64) },
	{ RGB( 80, 80,113), RGB( 44, 44, 64), RGB( 80, 97,113), RGB( 44, 64, 64) },
	{ RGB( 56, 68,113), RGB( 32, 40, 64), RGB( 56, 85,113), RGB( 44, 52, 64) },
	{ RGB(113, 56,  0), RGB( 64, 32,  0), RGB(113, 85,  0), RGB(113, 28,  0) },
	{ RGB(  0,113, 56), RGB(  0, 64, 32), RGB(  0,  0,  0), RGB( 32, 64, 56) },
	{ RGB(113, 28,  0), RGB( 64, 16,  0), RGB(113, 85,  0), RGB(113,  0, 28) },
	{ 0, 0 },
	{ RGB( 44, 44, 44), RGB( 32, 32, 32), RGB( 44, 64, 64), RGB( 44, 52, 64) },
};

ScreenDrawInfo *GetScreenDrawInfo(int screen)
{

	static const int screens[NumBackgrounds] = {
		8, 15, 22, 29, 36, 43, 50, 55, 62, 69, 70, 92, 999
	};

	for (int i = 0; i < NumBackgrounds; ++i) {
		if (screens[i] > screen)
			return (ScreenDrawInfo*)data + i;
	}

	return (ScreenDrawInfo*)data;
}

void UnpackLevel(int room)
{
	memset(ScreenTilesBuffer, 0x00, 0x2a8);

	unsigned char *endOfScreen = ScreenTilesBuffer + 0x2a8;
	unsigned char *p = SCREENS[room];

	for (int i = *p++; i > 0; i--, p += 4) {
		int xPos = *(p);
		int yPos = *(p + 1);

		unsigned char *pd = (unsigned char *)&ScreenTilesBuffer[yPos * 0x28 + xPos];
		#ifdef __DINGOO__
		unsigned char *ps = (unsigned char *)PATTERNS[*(p + 2) + (*(p + 3) << 8)];
		#else
		unsigned char *ps = (unsigned char *)PATTERNS[*(unsigned short *)(p + 2)];
		#endif

		int dy = *(ps + 1);
		int dx = *ps;

		ps += 2;

		for (int y = 0; y < dy; y++, pd += 0x28 - dx)
			for (int x = 0; x < dx; x++, ps++, pd++) {
				if (pd >= endOfScreen)
					break;

				if (x + xPos < 0x28 && *ps)
					*pd = *ps;
			}
	}
}

void BlitLevel(int room)
{
	for (int y = 0; y <= 16; y++)
		for (int x = 0; x <= 39; x++)
			PutTileI(x*8, y*8, ScreenTilesBuffer[y*0x28+x]);
}

void BlitLevelOutlines(int room)
{
	unsigned int shadow = GetScreenDrawInfo(room)->shadow;

	for (int y = 0; y <= 16; y++)
		for (int x = 0; x <= 39; x++)
			PutTileS(x*8, y*8, ScreenTilesBuffer[y*0x28+x], shadow);
}

void BlitBackground(int room)
{
	int background = GetScreenDrawInfo(room)->background;
	EraseBackground(background);

	for (int i = 0; i < 2; ++i) {
		short *lines = SCREENLINES[room];
		short count  = *(lines++);
		unsigned int color = (i == 1)
			? GetScreenDrawInfo(room)->line_light
			: GetScreenDrawInfo(room)->line_shadow;

		for (int j = 0; j < count; ++j, lines += 4) {
			int x1 = *(lines + 0);
			int y1 = *(lines + 1);
			int x2 = *(lines + 2);
			int y2 = *(lines + 3);

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
