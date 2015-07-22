#ifndef _OBJECT_ENEMY_H_
#define _OBJECT_ENEMY_H_

#include "object.h"

void Update_Static(TSHIP *gobj);
void Update_Random(TSHIP *gobj);
void Update_Kamikaze(TSHIP *gobj);
void Update_SparkleVertical(TSHIP *gobj);
void Update_CeilingCannon(TSHIP *gobj);
void Update_HomingMissile(TSHIP *gobj);
void Update_Cannon(TSHIP *gobj);
void Update_SparkleHorizontal(TSHIP *gobj);
void Update_Bonus(TSHIP *gobj);
void Update_Smoke(TSHIP *gobj);
void Update_Explosion(TSHIP *gobj);
void Update_Bridge(TSHIP *gobj);

void Update_Bullet(TSHIP *gobj);
void Update_HomingShot(TSHIP *gobj);
void Update_Shot(TSHIP *gobj);

void Update_Elevator(TSHIP *gobj);

#endif /* _OBJECT_ENEMY_H_ */
