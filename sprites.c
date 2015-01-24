/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS for(int A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <SDL/SDL.h>

#include "sprites.h"

#define SPRITE_NUMBER 57

typedef struct {
	int x, y, w, h;
	int dx, dy;
	int n;
} SPRITESET;

/* {x, y, w, h, dx, dy, n} from sprites.bmp */
SPRITESET SpriteSet[SPRITE_NUMBER] = {
	{ 17*16,  3*16, 32, 12,  0, 16, 7 }, /* ship */
	{ 12*16, 13*16, 40, 16, 40,  0, 2 }, /* chassis */

	{  0*16,  0*16, 16, 16, 16,  0, 3 }, /* enemy explosion */
	{  0*16,  1*16, 16, 16, 16,  0, 3 },
	{  0*16,  2*16, 16, 16, 16,  0, 3 },
	{  0*16,  3*16, 16, 16, 16,  0, 4 },
	{  0*16,  4*16,  8, 16,  8,  0, 2 }, /* breakable walls */
	{  0*16,  5*16, 16, 16, 16,  0, 3 }, /* walls explosion */
	{  0*16,  6*16, 16, 16, 16,  0, 4 },
	{  0*16,  7*16, 16, 16, 16,  0, 4 },
	{  0*16,  8*16, 16, 16, 16,  0, 4 },
	{  0*16,  9*16, 16, 16, 16,  0, 4 },
	{  0*16, 10*16, 16, 16, 16,  0, 2 },
	{  0*16, 11*16, 16, 16, 16,  0, 3 },
	{  0*16, 12*16, 16, 16, 16,  0, 2 },
	{  0*16, 13*16, 16, 16, 16,  0, 2 },
	{  0*16, 14*16, 16, 16, 16,  0, 2 },

	{ 12*16,  5*16, 16,  6, 16,  0, 2 }, /* horizontal sparkle */
	{ 12*16,  6*16,  8, 14,  8,  0, 2 }, /* vertical sparkle */

	{  5*16,  0*16, 16, 16, 16,  0, 4 },
	{  5*16,  1*16, 16, 16, 16,  0, 2 },

	{ 12*16,  7*16, 48,  8,  0,  0, 1 }, /* elevator platform */

	{  5*16,  2*16, 16, 16, 16,  0, 4 },

	{ 12*16, 14*16, 32, 12, 21,  0, 4 }, /* chassis explosion */

	{  5*16,  3*16, 16, 16, 16,  0, 4 },
	{  5*16,  4*16, 16, 16, 16,  0, 6 },
	{  5*16,  5*16, 16, 16, 16,  0, 2 },
	{  5*16,  6*16, 16, 16, 16,  0, 4 }, /* not used ? */
	{  5*16,  7*16, 16, 16, 16,  0, 4 },
	{  5*16,  8*16, 16, 16, 16,  0, 4 },
	{  5*16,  9*16, 16, 16, 16,  0, 6 },
	{  5*16, 10*16, 16, 16, 16,  0, 4 },
	{  5*16, 11*16, 16, 16, 16,  0, 6 }, /* spawns enemy */
	{  5*16, 12*16, 16, 16, 16,  0, 4 },
	{  5*16, 13*16, 16, 16, 16,  0, 4 },
	{  5*16, 14*16, 16, 16, 16,  0, 4 }, /* not used ? */

	{ 12*16,  0*16, 16, 16, 16,  0, 4 },
	{ 12*16,  1*16, 16, 16, 16,  0, 6 },
	{ 12*16,  2*16, 16, 16, 16,  0, 4 },
	{ 12*16,  3*16, 16, 16, 16,  0, 2 },

	{ 12*16,  8*16, 24,  7, 24,  0, 2 }, /* missile 1 */
	{ 12*16,  9*16, 24,  7, 24,  0, 2 }, /* missile 2 */

	{ 12*16,  4*16, 16, 16, 16,  0, 4 }, /* cannon, spawns bullet */
	{ 12*16, 10*16,  4,  4,  0,  0, 1 }, /* bullet */
	{ 12*16, 11*16, 40,  8,  0,  0, 1 }, /* platform */
	{ 12*16, 12*16, 72, 12,  0,  0, 1 }, /* Opera Soft logo */

	{   117,    17, 13, 13, 13,  0, 5 }, /* smoke */
	{   151,    33, 15, 15,  0,  0, 1 }, /* facebook logo */
	{   167,    33, 15, 15,  0,  0, 1 }, /* twitter logo */
	{   243,    86, 26, 19,  0, 19, 4 }, /* two astronauts */
	{   372,    52,  8,  2,  0,  3, 2 }, /* machine gun bullet */

	{   334,  3*16, 32, 12,  0, 16, 7 }, /* machine gun ship */
	{   165,    91, 11, 12,  0,  0, 1 }, /* extra HP */
	{   303,  3*16, 32, 12,  0, 16, 7 }, /* rocket ship */
	{   309,    12, 14,  8,  0,  8, 4 }, /* rocket */
	{   375,   145, 16, 16,  0, 18, 5 }, /* BFG */
	{   287,   204, 10, 10, 11,  0, 4 }  /* BFG shot */
};

extern SDL_Surface *small_screen;
SDL_Surface *sprites;
SDL_Surface *tiles;

int GetSpriteW(int index)
{
	return SpriteSet[index].w;
}

int GetSpriteH(int index)
{
	return SpriteSet[index].h;
}

void PutSpriteI(int x, int y, int index, int frame)
{
	SDL_Rect src, dst;

	if (index >= SPRITE_NUMBER)
		return;

	if (frame >= SpriteSet[index].n)
		return;

	dst.x = x;
	dst.y = y;

	src.x = SpriteSet[index].x + SpriteSet[index].dx * frame;
	src.y = SpriteSet[index].y + SpriteSet[index].dy * frame;
	src.w = SpriteSet[index].w;
	src.h = SpriteSet[index].h;

	SDL_BlitSurface(sprites, &src, small_screen, &dst);
}

#define SET_GAME_AREA_POINT(x, y, color) \
{ if ((x) >= 0 && (x) < SCREEN_WIDTH && (y) >= 0 && (y) < ACTION_SCREEN_HEIGHT) \
	putpixel(small_screen, x, y, color); }

/*
 * getpixel() and putpixel() are taken from
 * http://www.libsdl.org/release/SDL-1.2.15/docs/html/guidevideo.html
 */

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
      int bpp = surface->format->BytesPerPixel;
      /* Here p is the address to the pixel we want to retrieve */
      Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

      switch(bpp) {
      case 1:
		return *p;

      case 2:
		return *(Uint16 *)p;

      case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;

      case 4:
		return *(Uint32 *)p;

      default:
		return 0;       /* shouldn't happen, but avoids warnings */
      }
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *)p = pixel;
		break;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}

int GetSpritePixel(int x, int y, int index, int frame)
{
	return getpixel(sprites,
			x + SpriteSet[index].x + SpriteSet[index].dx * frame,
			y + SpriteSet[index].y + SpriteSet[index].dy * frame);
}

/* Blit a 'shadow' of the sprite */
void PutSpriteS(int x, int y, int index, int frame, int color)
{
	int w = GetSpriteW(index);
	int h = GetSpriteH(index);
	int dx, dy;

	for (dy = 0; dy < h; dy++)
		for (dx = 0; dx < w; dx++) {
			if (GetSpritePixel(dx, dy, index, frame)) {
				SET_GAME_AREA_POINT(x + dx - 1, y + dy, color);
				SET_GAME_AREA_POINT(x + dx + 1, y + dy, color);
				SET_GAME_AREA_POINT(x + dx, y + dy - 1, color);
				SET_GAME_AREA_POINT(x + dx, y + dy + 1, color);
				SET_GAME_AREA_POINT(x + dx, y + dy + 2, color);

			}
		}
}

/* Tiles are grouped 40x7, 8x8 each tile*/
void PutTileI(int x, int y, int index)
{
	SDL_Rect src, dst;

	if (index > 256)
		return;

	dst.x = x;
	dst.y = y;

	src.x = index % 40 * 8;
	src.y = index / 40 * 8;
	src.w = 8;
	src.h = 8;

	SDL_BlitSurface(tiles, &src, small_screen, &dst);
}

int GetTilePixel(int x, int y, int index)
{
	return getpixel(tiles,
			x + index % 40 * 8,
			y + index / 40 * 8);
}

void PutTileS(int x, int y, int index, int color)
{
	int w = 8;
	int h = 8;
	int dx, dy;

	for (dy = 0; dy < h; dy++)
		for (dx = 0; dx < w; dx++) {
			if (GetTilePixel(dx, dy, index)) {
				SET_GAME_AREA_POINT(x + dx - 1, y + dy, color);
				SET_GAME_AREA_POINT(x + dx + 1, y + dy, color);
				SET_GAME_AREA_POINT(x + dx, y + dy - 1, color);
				SET_GAME_AREA_POINT(x + dx, y + dy + 1, color);
				SET_GAME_AREA_POINT(x + dx, y + dy + 2, color);

			}
		}
}

void PutBgI(int x, int y, int index)
{
	SDL_Rect src, dst;

	if (index > 9)
		return;

	dst.x = x;
	dst.y = y;

	src.x = index * 16;
	src.y = 6 * 16;
	src.w = 16;
	src.h = 16;

	SDL_BlitSurface(tiles, &src, small_screen, &dst);
}

void PutLetterI(int x, int y, int index)
{
	SDL_Rect src, dst;

	if (index > 256)
		return;

	dst.x = x;
	dst.y = y;

	src.x = index % 40 * 8;
	src.y = index / 40 * 8 + 64;
	src.w = 8;
	src.h = 8;

	SDL_SetColorKey(tiles, 0, 0);
	SDL_BlitSurface(tiles, &src, small_screen, &dst);
	SDL_SetColorKey(tiles, SDL_SRCCOLORKEY, 0);
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

void PutString(int x, int y, char *p)
{
	while (*p != 0) {
		PutLetterI(x, y, AdjustAscii(*p));
		p += 1;
		x += 8;
	}
}


void PutStream(int x, int y, unsigned char *p)
{
	while (*p != 0) {
		PutLetterI(x, y, *p);
		p += 1;
		x += 8;
	}
}

void EraseBackground(int color)
{
	SDL_Rect dst;

	dst.x = 0;
	dst.y = 0;
	dst.w = SCREEN_WIDTH;
	dst.h = ACTION_SCREEN_HEIGHT;

	SDL_FillRect(small_screen, &dst, color);
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
			SET_GAME_AREA_POINT(x1, i, color);
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
			SET_GAME_AREA_POINT(i, y1, color);
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

void LoadSprites()
{
	sprites = SDL_LoadBMP("graphics/sprites.bmp");
	SDL_SetColorKey(sprites, SDL_SRCCOLORKEY, 0);

	tiles = SDL_LoadBMP("graphics/tiles.bmp");
	SDL_SetColorKey(tiles, SDL_SRCCOLORKEY, 0);
}
