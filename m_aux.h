#ifndef _M_AUX_H_
#define _M_AUX_H_

void UnpackLevel(int room);
void BlitLevel(int room);
void BlitLevelOutlines(int room);
void BlitBackground(int room);

typedef struct {
	unsigned int background;
	unsigned int shadow;
	unsigned int line_light;
	unsigned int line_shadow;
} ScreenDrawInfo;

ScreenDrawInfo *GetScreenDrawInfo(int screen);

typedef struct {
	int x, y;
	float radius;
	float r, g, b;
} Light;

#define MAX_LIGHTS 13

#endif //_M_AUX_H_
