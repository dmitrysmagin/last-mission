
#include <string.h>
#include <stdlib.h>
#include "m_aux.h"
#include "m_data.h"
#include "m_gfx_data.h"

#define SET_GAME_AREA_POINT(x, y, color) \
{ if ((x) >= 0 && (x) < SCREEN_WIDTH && (y) >= 0 && (y) < ACTION_SCREEN_HEIGHT) \
	*(unsigned char *)((y) * SCREEN_WIDTH + (x) + pScreenBuffer) = color; }

#define SET_SCREEN_POINT(x, y, color) \
{ if ((x) >= 0 && (x) < SCREEN_WIDTH && (y) >= 0 && (y) < SCREEN_HEIGHT) \
	*(unsigned char *)((y) * SCREEN_WIDTH + (x) + pScreenBuffer) = color; }

#define SET_SCREEN_POINT_IF(condition, x, y, color) \
{ if ((x) >= 0 && (x) < SCREEN_WIDTH && (y) >= 0 && (y) < SCREEN_HEIGHT && (condition)) \
	*(unsigned char *)((y) * SCREEN_WIDTH + (x) + pScreenBuffer) = color; }

// external data in m_core.c
extern unsigned char *pScreenBuffer;
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

unsigned char AdjustAscii(unsigned char a)
{
	if (a <= 0x5a) {
		if (a >= 0x41)
			return a - 0x41 + 0xc;

		if (a == 0x20)
			return 0;

		return a - 0x30 + 1;
	}

	return 0;
}

void DrawLine(int x1, int y1, int x2, int y2, unsigned char color)
{
	const int deltaX = abs(x2 - x1);
	const int deltaY = abs(y2 - y1);
	static int temp, i;

	if (deltaX == 0) {
		if (x1 < 0 || x1 >= SCREEN_WIDTH)
			return;

		if (y1 > y2) {
			temp = y2;
			y2 = y1;
			y1 = temp;
		}

		if (y1 < 0)
			y1 = 0;

		if (y2 >= ACTION_SCREEN_HEIGHT)
			y2 = ACTION_SCREEN_HEIGHT - 1;

		for (i = y1; i <= y2; ++i) {
			*(pScreenBuffer + i * SCREEN_WIDTH + x1) = color;
		}
	} else if (deltaY == 0) {
		if (y1 < 0 || y1 >= ACTION_SCREEN_HEIGHT)
			return;

		if (x1 > x2) {
			temp = x2;
			x2 = x1;
			x1 = temp;
		}

		if (x1 < 0)
			x1 = 0;

		if (x2 >= SCREEN_WIDTH)
			x2 = SCREEN_WIDTH - 1;

		for (i = x1; i <= x2; ++i) {
			*(pScreenBuffer + y1 * SCREEN_WIDTH + i) = color;
		}
	} else {
		const int signX = x1 < x2 ? 1 : -1;
		const int signY = y1 < y2 ? 1 : -1;

		int error = deltaX - deltaY;
		SET_GAME_AREA_POINT(x2, y2, color);

		while (x1 != x2 || y1 != y2) {
			SET_GAME_AREA_POINT(x1, y1, color);
			const int error2 = error * 2;

			if (error2 > -deltaY) {
				error -= deltaY;
				x1 += signX;
			}

			if (error2 < deltaX) {
				error += deltaX;
				y1 += signY;
			}
		}
	}
}

void DrawRect(int x, int y, int width, int height, unsigned char color)
{
	DrawLine(x, y, x + width, y, color);
	DrawLine(x + width, y, x + width, y + height, color);
	DrawLine(x + width, y + height, x, y + height, color);
	DrawLine(x, y + height, x, y, color);
}

void PutGeneric256(int x, int y, int xSize, int ySize, unsigned char *p)
{
	static int dx, dy;

	for (dy = 0; dy < ySize; dy++)
		for (dx = 0; dx < xSize; dx++) {
			SET_SCREEN_POINT_IF(*p != 0, x + dx, y + dy, *p);
			p++;
		}
}

void PutGeneric256Outline(int x, int y, int xSize, int ySize, unsigned char *p, unsigned char shadow)
{
	static int dx, dy;

	for (dy = 0; dy < ySize; dy++)
		for (dx = 0; dx < xSize; dx++) {
			if (*p) {
				SET_GAME_AREA_POINT(x + dx - 1, y + dy, shadow);
				SET_GAME_AREA_POINT(x + dx + 1, y + dy, shadow);
				SET_GAME_AREA_POINT(x + dx, y + dy - 1, shadow);
				SET_GAME_AREA_POINT(x + dx, y + dy + 1, shadow);
				SET_GAME_AREA_POINT(x + dx, y + dy + 2, shadow);
			}

			p++;
		}
}

void PutGeneric256NoAlpha(int x, int y, int xSize, int ySize, unsigned char *p)
{
	static int dx, dy;

	for (dy = 0; dy < ySize; dy++)
		for (dx = 0; dx < xSize; dx++) {
			SET_SCREEN_POINT(x + dx, y + dy, *p);
			p++;
		}
}

void PutSprite(int x, int y,unsigned char *p)
{
	PutGeneric256(x, y, *p, *(p + 1), p + 2);
}

void PutSpriteOutline(int x, int y, unsigned char *p, unsigned char shadow)
{
	PutGeneric256Outline(x, y, *p, *(p + 1), p + 2, shadow);
}

void PutTile(int x, int y, unsigned char *p)
{
	PutGeneric256(x, y, 8, 8, p);
}

void PutLetter(int x, int y, unsigned char a)
{
	PutGeneric256NoAlpha(x, y, 8, 8, &Font256[a*64]);
}

void PutString(int x, int y, char *p)
{
	while (*p != 0) {
		PutLetter(x, y, AdjustAscii(*p));
		p += 1;
		x += 8;
	}
}


void PutStream(int x, int y, unsigned char *p)
{
	while (*p != 0) {
		PutLetter(x, y, *p);
		p += 1;
		x += 8;
	}
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
			PutTile(x*8, y*8, (unsigned char *)&Tiles256[ScreenTilesBuffer[y*0x28+x]*64]);
}

void BlitLevelOutlines()
{
	unsigned char shadow = GetScreenDrawInfo(ship_cur_screen)->shadow;

	for (int y = 0; y <= 16; y++)
		for (int x = 0; x <= 39; x++)
			PutGeneric256Outline(x*8, y*8, 8, 8, (unsigned char *)&Tiles256[ScreenTilesBuffer[y*0x28+x]*64], shadow);
}

void EraseBackground()
{
	memset(pScreenBuffer, 0, SCREEN_WIDTH*ACTION_SCREEN_HEIGHT);
}

void BlitBackground()
{
	int background = GetScreenDrawInfo(ship_cur_screen)->background;
	memset(pScreenBuffer, background, SCREEN_WIDTH*ACTION_SCREEN_HEIGHT);

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
				PutSprite(x*16 - 4, y*16 - 8, pBgSprites[SkyMap[y][x]]);
	}
}
