#ifndef _SPRITES_H_
#define _SPRITES_H_

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define ACTION_SCREEN_HEIGHT 136

void LoadSprites();

void PutSpriteI(int x, int y, int index, int frame);
void PutSpriteS(int x, int y, int index, int frame, int color);
int GetSpriteW(int index);
int GetSpriteH(int index);

void PutTileI(int x, int y, int index);
void PutTileS(int x, int y, int index, int color);

void PutBgI(int x, int y, int index);

void PutString(int x, int y, char *p);
void PutStream(int x, int y, unsigned char *p);

void EraseBackground(int color);

void DrawLine(int x1, int y1, int x2, int y2, unsigned char color);
void DrawRect(int x, int y, int width, int height, unsigned char color);

#endif /* _SPRITES_H_ */
