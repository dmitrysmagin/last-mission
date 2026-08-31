#ifndef _ENGINE_COORDINATES_H_
#define _ENGINE_COORDINATES_H_

#include "room.h"

typedef struct {
    // Get tile at room-local coordinates
    int (*get_tile)(ROOMCONTAINER *rc, int x, int y);
    
    // Set tile at room-local coordinates
    void (*set_tile)(ROOMCONTAINER *rc, int x, int y, int i);
    
    // Convert room-local tile coords to world tile index
    int (*tile_to_world)(int room_x, int room_y, int gx, int gy);
    
    // Convert world tile index to room-local coords
    void (*world_to_tile)(int world_idx, int *gx, int *gy, int *room_x, int *room_y);
    
    // Get world pixel coordinates for a tile
    void (*tile_to_world_pixel)(int gx, int gy, int room_x, int room_y, int *wx, int *wy);
    
    // Get room container for world pixel coordinates
    ROOMCONTAINER *(*get_container_for_world)(int wx, int wy);
    
    // Get tile at world pixel coordinates
    int (*get_tile_at_world)(int wx, int wy);
    
    // Set tile at world pixel coordinates
    void (*set_tile_at_world)(int wx, int wy, int i);
    
    // Get viewport bounds in world pixels
    void (*get_viewport)(int *x, int *y, int *w, int *h);
} ICoordinateSystem;

int Coords_GetTile(ROOMCONTAINER *rc, int x, int y);
void Coords_SetTile(ROOMCONTAINER *rc, int x, int y, int i);

#endif /* _ENGINE_COORDINATES_H_ */
