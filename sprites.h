#ifndef _SPRITES_H_
#define _SPRITES_H_

void LoadSprites();

void PutSpriteI(int x, int y, int index, int frame);
void PutSpriteS(int x, int y, int index, int frame, int color);
int GetSpriteW(int index);
int GetSpriteH(int index);

#endif /* _SPRITES_H_ */
