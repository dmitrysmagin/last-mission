
#include <string.h>
#include <stdlib.h>

#include "sprites.h"
#include "m_aux.h"
#include "m_data.h"
#include "m_gfx_data.h"

// external data in m_core.c
extern unsigned char ScreenTilesBuffer[0x2a8];
extern unsigned char ship_cur_screen;

#define NumBackgrounds 13

ScreenDrawInfo *GetScreenDrawInfo(int screen)
{
	static ScreenDrawInfo data[NumBackgrounds] = {
		{ 20, 18, 244, 246 },
		{ 112, 112 + 72, 113, 255 },
		{ 128, 128 + 72, 151, 246 },
		{ 161, 161 + 72, 163, 160 },
		{ 114, 114 + 72, 115, 111 },
		{ 126, 126 + 72, 150, 246 },
		{ 152, 152 + 72, 174, 244 },
		{ 151, 151 + 72, 150, 246 },
		{ 114, 114 + 72, 115, 113 },
		{ 122, 122 + 72, 254, 219 },
		{ 113, 113 + 72, 115, 111 },
		{ 0, 0 },
		{ 19, 18, 244, 246 },
	};

	static const int screens[NumBackgrounds] = {
		8, 15, 22, 29, 36, 43, 50, 55, 62, 69, 70, 92, 999
	};

	for (int i = 0; i < NumBackgrounds; ++i) {
		if (screens[i] > screen)
			return (ScreenDrawInfo*)data + i;
	}

	return (ScreenDrawInfo*)data;
}

void UnpackLevel()
{
	memset(ScreenTilesBuffer, 0x00, 0x2a8);

	unsigned char *endOfScreen = ScreenTilesBuffer + 0x2a8;
	unsigned char *p = SCREENS[ship_cur_screen];

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

void BlitLevel()
{
	for (int y = 0; y <= 16; y++)
		for (int x = 0; x <= 39; x++)
			PutTileI(x*8, y*8, ScreenTilesBuffer[y*0x28+x]);
}

void BlitLevelOutlines()
{
	unsigned char shadow = GetScreenDrawInfo(ship_cur_screen)->shadow;

	for (int y = 0; y <= 16; y++)
		for (int x = 0; x <= 39; x++)
			PutTileS(x*8, y*8, ScreenTilesBuffer[y*0x28+x], shadow);
}

void BlitBackground()
{
	int background = GetScreenDrawInfo(ship_cur_screen)->background;
	EraseBackground(background);

	for (int i = 0; i < 2; ++i) {
		short *lines = SCREENLINES[ship_cur_screen];
		short count  = *(lines++);
		unsigned char color = (i == 1)
			? GetScreenDrawInfo(ship_cur_screen)->line_light
			: GetScreenDrawInfo(ship_cur_screen)->line_shadow;

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

	if (ship_cur_screen > 69 && ship_cur_screen < 92) {
		for (int y = 0; y <= 8; y++)
			for (int x = 0; x <= 20; x++)
				PutBgI(x*16 - 4, y*16 - 8, SkyMap[y][x]);
	}
}
