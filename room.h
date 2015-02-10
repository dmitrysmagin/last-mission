#ifndef _ROOM_H_
#define _ROOM_H_

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

#endif //_ROOM_H_
