#ifndef _OBJECT_LASER_H_
#define _OBJECT_LASER_H_

#include "object.h"

typedef struct SDL_Surface SDL_Surface;

void DoLaser(TSHIP *ship);
void ResetLaser();
int UpdateLaser(int i);
void BlitLaser(SDL_Surface *surface, TSHIP *gobj);
void BlitLaserStatus();

void Update_Laser(TSHIP *gobj);

#endif /* _OBJECT_LASER_H_ */
