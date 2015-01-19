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
				putpixel(small_screen, x + dx - 1, y + dy, color);
				putpixel(small_screen, x + dx + 1, y + dy, color);
				putpixel(small_screen, x + dx, y + dy - 1, color);
				putpixel(small_screen, x + dx, y + dy + 1, color);
				putpixel(small_screen, x + dx, y + dy + 2, color);

			}
		}
}

void LoadSprites()
{
	sprites = SDL_LoadBMP("graphics/sprites.bmp");
	//printf("sprites bpp: %d\n", sprites->format->BitsPerPixel);

	SDL_SetColorKey(sprites, SDL_SRCCOLORKEY, 0);
}
