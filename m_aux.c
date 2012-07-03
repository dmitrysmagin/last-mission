
#include <string.h>
#include "m_aux.h"
#include "m_data.h"
#include "m_gfx_data.h"

// external data in m_core.c
extern unsigned char *pScreenBuffer;
extern unsigned char ScreenTilesBuffer[0x2a8];
extern unsigned char ship_cur_screen;


void word2string(unsigned int value, char *buffer)
{
	char *p = buffer;
	static unsigned int _smth[5] = {10000, 1000, 100, 10, 1};


	if (value == 0) {
		*(unsigned int *)(p) = 0x002030;
		return;
	}


	for(int c = 0; c < 5; c++) {
		if(value / _smth[c] + p == buffer) continue; // skip beginning zeroes
		*p = value / _smth[c] + 0x30;
		p++;
		value %= _smth[c];
	}

	#ifdef __DINGOO__
	*p = 0x20;
	*(p+1) = 0;
	#else
	*(unsigned short *)(p) = 0x20;
	#endif

}

void Int2ZString(int digit, int num_of_digits, char *buffer)
{
	static int _smth[8] = {10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};

	if(num_of_digits > 8) num_of_digits = 8;

	memset(buffer, 0x30, num_of_digits);

	for(int i = 8 - num_of_digits; i < 8; i++) {
		*(unsigned char *)(buffer + i - 8 + num_of_digits) = digit / _smth[i] | 0x30;
		*(unsigned char *)(buffer + i - 8 + num_of_digits + 1) = 0;
		digit %= _smth[i];
	}
}

unsigned char AdjustAscii(unsigned char a)
{
	if(a <= 0x5a) {
		if (a >= 0x41) return a - 0x41 + 0xc;
		if (a == 0x20) return 0;
		return a - 0x30 + 1;
	}

	return 0;
}

void PutGeneric(int x, int y, int xSize, int ySize, unsigned char *p)
{
	static unsigned char smth[4] = {6, 4, 2, 0};
	static unsigned char CGA_Palette[4] = {0, 3, 5, 7};

	static int dx, dy;

	for(dy = 0; dy < ySize; dy++)
		for(dx = 0; dx < xSize; dx++) {
			if(x + dx >= 0) {
				if(x + dx < 320) {
					if(y + dy >= 0) {
						if(y + dy < 200) {
							*(unsigned char *)((y + dy) * 320 + x + dx + pScreenBuffer) = CGA_Palette[(*p >> smth[dx & 3]) & 3];
						}
					}
				}
			}
			if((dx & 3) == 3) p++;
		}
}
void PutGeneric256(int x, int y, int xSize, int ySize, unsigned char *p)
{
	static int dx, dy;

	for(dy = 0; dy < ySize; dy++)
		for(dx = 0; dx < xSize; dx++) {
			if(x + dx >= 0) {
				if(x + dx < 320) {
					if(y + dy >= 0) {
						if(y + dy < 200) {
							if(*p != 0) *(unsigned char *)((y + dy) * 320 + x + dx + pScreenBuffer) = *p;
						}
					}
				}
			}
			p++;
		}
}

void PutBlank(int x, int y, unsigned char *p)
{

	static int dx, dy, xSize, ySize;

	xSize = *p;
	ySize = *(p + 1);
	p += 2;

	for(dy = 0; dy < ySize; dy++)
		for(dx = 0; dx < xSize; dx++) {
			if(x + dx >= 0) {
				if(x + dx < 320) {
					if(y + dy >= 0) {
						if(y + dy < 200) {
							if(*p != 0)	*(unsigned char *)((y + dy) * 320 + x + dx + pScreenBuffer) = 0;
						}
					}
				}
			}
			p++;
		}
}

void PutSprite(int x, int y,unsigned char *p)
{
	PutGeneric256(x, y, *p, *(p + 1), p + 2);
}

void PutTile(int x, int y, unsigned char *p)
{
	PutGeneric256(x, y, 8, 8, p);
}

void PutLetter(int x, int y, unsigned char a)
{
	PutGeneric256(x, y, 8, 8, &Font256[a*64]);
}


void PutString(int x, int y, char *p)
{
	while(*p != 0)
	{
		PutLetter(x, y, AdjustAscii(*p));
		p += 1;
		x += 8;
	}
}


void PutStream(int x, int y, unsigned char *p)
{
	while(*p != 0)
	{
		PutLetter(x, y, *p);
		p += 1;
		x += 8;
	}
}

// simple cache, may speed up things TEST IT TEST IT

#if defined(__DINGUX__) || defined(__DINGOO__)
int level_cache_fl = 0;
unsigned char level_cache[17*8*320];
#endif

void UnpackLevel()
{

	memset(ScreenTilesBuffer, 0x00, 0x2a8);

	unsigned char *p = SCREENS[ship_cur_screen];
	for(int i = *p++; i > 0; i--, p += 4)
	{

		unsigned char *pd = (unsigned char *)&ScreenTilesBuffer[*(p + 1) * 0x28 + *p];
		#ifdef __DINGOO__
		unsigned char *ps = (unsigned char *)PATTERNS[*(p + 2) + (*(p + 3) << 8)];
		#else
		unsigned char *ps = (unsigned char *)PATTERNS[*(unsigned short *)(p + 2)];
		#endif

		int dy = *(ps + 1);
		int dx = *ps;

		ps += 2;

		for(int y = 0; y < dy; y++, pd += 0x28 - dx)
			for(int x = 0; x < dx; x++, ps++, pd++) *pd = *ps;
	}
	#if defined(__DINGUX__) || defined(__DINGOO__)
	level_cache_fl = 1;
	#endif
}

void BlitLevel()
{
	#if defined(__DINGUX__) || defined(__DINGOO__)
	if(level_cache_fl == 1)
	{
	#endif
		for(int y = 0; y <= 16; y++)
			for(int x = 0; x <= 39; x++)
				PutTile(x*8, y*8, (unsigned char *)&Tiles256[ScreenTilesBuffer[y*0x28+x]*64]);
	#if defined(__DINGUX__) || defined(__DINGOO__)
		memcpy(level_cache, pScreenBuffer, 17*8*320);
		level_cache_fl = 0;
	} else memcpy(pScreenBuffer, level_cache, 17*8*320);
	#endif
}

void BlitBackground()
{

	// implement later
	memset(pScreenBuffer, 0, 320*136);

	// LATER: all screens above 69 should have stars as background
	//if(ship_cur_screen > 7) return;

	//for(int y = 0; y <= 8; y++)
	//	for(int x = 0; x <= 20; x++)
	//		PutSprite(x*16 - 4 /*BgOffsets[ship_cur_screen]*/, y*16 - 8, pBgSprites[BgMap01[y][x]]);

}

int randseed = 0x342a;

typedef union
{
	int ax;
	short al;
	short ah;
} A;
typedef union
{
	int bx;
	short bl;
	short bh;
} B;
typedef union
{
	int dx;
	short dl;
	short dh;
} D;

// just emulate that old asm routine :)
int RandomInt()
{
	A a;
	B b;
	D d;

	d.dx = randseed;
	b.bh = d.dl;
	b.bl = -3;
	a.ah = d.dh;

	b.bx -= d.dx;

	if(b.bx < 0) a.ah -= 1;

	b.bx -= d.dx;

	if(b.bx < 0) a.ah -= 1;

	d.dl = a.ah;
	d.dh = 0;

	b.bx -= d.dx;

	if(b.bx < 0) b.bx += 1;

	randseed = b.bx;

	return randseed;
}

// Set initial seed value, if 0 set to default 0x342a cause it's needed for demo mode
void Randomize(int seed)
{
	if(seed == 0) randseed = 0x342a; else randseed = seed;
}
