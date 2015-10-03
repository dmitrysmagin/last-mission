#ifndef _OBJECT_LASER_H_
#define _OBJECT_LASER_H_

#include "object.h"

/* FIXME: eliminate later */
extern int laser_dir;

void DoLaser();
void ResetLaser();
int UpdateLaser(int i);
void BlitLaser();
void BlitLaserStatus();

void Update_Laser(TSHIP *gobj);

#endif /* _OBJECT_LASER_H_ */