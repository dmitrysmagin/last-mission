/*

	m_aux.h 

*/
#ifndef _M_AUX_H_
#define _M_AUX_H_

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define ACTION_SCREEN_HEIGHT 136

void word2string(unsigned int value, char *buffer);
void Int2ZString(int digit, int num_of_digits, char *buffer);
unsigned char AdjustAscii(unsigned char a);
void DrawLine(int x1, int y1, int x2, int y2, unsigned char color);
void DrawRect(int x, int y, int width, int height, unsigned char color);
void PutGeneric(int x, int y, int xSize, int ySize, unsigned char *p);
void PutBlank(int x, int y, unsigned char *p);
void PutSprite(int x, int y,unsigned char *p);
void PutSpriteOutline(int x, int y, unsigned char *p, unsigned char shadow);
void PutTile(int x, int y, unsigned char *p);
void PutLetter(int x, int y, unsigned char a);
void PutString(int x, int y, char *p);
void PutStream(int x, int y, unsigned char *p);
void UnpackLevel();
void BlitLevel();
void BlitLevelOutlines();
void BlitBackground();
void EraseBackground();
int RandomInt();
void Randomize(int seed);

typedef struct
{
	unsigned char background;
	unsigned char shadow;
	unsigned char line_light;
	unsigned char line_shadow;
} ScreenDrawInfo;

ScreenDrawInfo *GetScreenDrawInfo(int screen);

typedef struct 
{
	int x, y;
	float radius;
	float r, g, b;
} Light;

#define MAX_LIGHTS 13

#endif //_M_AUX_H_
