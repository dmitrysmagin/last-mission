#ifndef _ENGINE_COLLISION_H_
#define _ENGINE_COLLISION_H_

#include "room.h"
#include "object.h"

typedef struct {
    // Check if object overlaps with tile at given position
    int (*check_tile_collision)(int x, int y, TSHIP *gobj);
    
    // Check if two objects overlap
    int (*check_object_collision)(int x1, int y1, TSHIP *gobj1, int x2, int y2, TSHIP *gobj2);
    
    // Check if object touches any solid tile
    int (*check_solid_collision)(int x, int y, TSHIP *gobj);
    
    // Get tile collision flags
    int (*get_tile_flags)(int tile);
    
    // Mark tile as destroyed
    void (*mark_tile_destroyed)(int wx, int wy);
    
    // Check if tile is marked as destroyed
    int (*is_tile_destroyed)(int wx, int wy);
} ICollisionSystem;

int Collision_CheckTile(int x, int y, TSHIP *gobj);
int Collision_CheckOverlap(int x1, int y1, TSHIP *gobj1, int x2, int y2, TSHIP *gobj2);
int Collision_CheckSolid(int x, int y, TSHIP *gobj);

#endif /* _ENGINE_COLLISION_H_ */
