#ifndef _ENGINE_COLLISION_H_
#define _ENGINE_COLLISION_H_

#include "engine-interface.h"
#include "object.h"

// Main collision dispatcher - uses engine from game state
int Collision_CheckTile(int x, int y, TSHIP *gobj);
int Collision_CheckOverlap(int x1, int y1, TSHIP *gobj1, int x2, int y2, TSHIP *gobj2);
int Collision_CheckSolid(int x, int y, TSHIP *gobj);

// Bounds checking helpers
int Collision_CheckBounds(int x, int y, int w, int h, IGameEngine *engine);
int Collision_CheckObjectBounds(TSHIP *obj, IGameEngine *engine);

// Per-engine bounds checking
int Collision_FlipScreenCheckBounds(int x, int y, int w, int h);
int Collision_ScrollingCheckBounds(int x, int y, int w, int h);

#endif /* _ENGINE_COLLISION_H_ */
