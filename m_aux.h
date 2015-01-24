#ifndef _M_AUX_H_
#define _M_AUX_H_

void UnpackLevel();
void BlitLevel();
void BlitLevelOutlines();
void BlitBackground();

typedef struct {
	unsigned char background;
	unsigned char shadow;
	unsigned char line_light;
	unsigned char line_shadow;
} ScreenDrawInfo;

ScreenDrawInfo *GetScreenDrawInfo(int screen);

typedef struct {
	int x, y;
	float radius;
	float r, g, b;
} Light;

#define MAX_LIGHTS 13

#endif //_M_AUX_H_
