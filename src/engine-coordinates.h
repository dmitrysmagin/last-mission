#ifndef _ENGINE_COORDINATES_H_
#define _ENGINE_COORDINATES_H_

#include "engine-interface.h"
#include "object.h"

// World to Screen conversion
// For flip-screen: world coords are room-local, screen coords are same
// For scrolling: subtract camera offset from world coords
int Coords_WorldToScreenX(int worldX, IGameEngine *engine);
int Coords_WorldToScreenY(int worldY, IGameEngine *engine);

// Screen to World conversion
int Coords_ScreenToWorldX(int screenX, IGameEngine *engine);
int Coords_ScreenToWorldY(int screenY, IGameEngine *engine);

// Object coordinate helpers
// Get screen X position for rendering (converts from world coords if needed)
int Coords_ObjectGetScreenX(TSHIP *obj, IGameEngine *engine);
int Coords_ObjectGetScreenY(TSHIP *obj, IGameEngine *engine);

// Check if object is within viewport bounds
int Coords_ObjectInViewport(TSHIP *obj, IGameEngine *engine);

// Room boundary helpers
int Coords_GetRoomWidth(void);
int Coords_GetRoomHeight(void);

// Tile coordinate helpers
int Coords_WorldToTileX(int worldX);
int Coords_WorldToTileY(int worldY);

#endif /* _ENGINE_COORDINATES_H_ */
