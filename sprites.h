#ifndef _SPRITES_H_
#define _SPRITES_H_

#include "video.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define ACTION_SCREEN_HEIGHT 136

#if GAME_SCREEN_BPP == 16
#define RGB(r, g, b) ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3)
#elif GAME_SCREEN_BPP == 32
#define RGB(r, g, b) (((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff))
#endif

void LoadSprites();

void PutSpriteI(int x, int y, int index, int frame);
void PutSpriteS(int x, int y, int index, int frame, unsigned int color);
int GetSpriteW(int index);
int GetSpriteH(int index);

void PutTileI(int x, int y, int index);
void PutTileS(int x, int y, int index, unsigned int color);

void PutBgI(int x, int y, int index);

void PutString(int x, int y, char *p);
void PutStream(int x, int y, unsigned char *p);

void EraseBackground(unsigned int color);
void FillScreen(int x, int y, int w, int h, unsigned int color);

void DrawLine(int x1, int y1, int x2, int y2, unsigned int color);
void DrawRect(int x, int y, int width, int height, unsigned int color);

void PutPixel(int x, int y, unsigned int color);

#endif /* _SPRITES_H_ */
