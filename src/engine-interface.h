#ifndef _ENGINE_INTERFACE_H_
#define _ENGINE_INTERFACE_H_

#include "video.h"
#include "room.h"

typedef struct IGameEngine IGameEngine;

typedef void (*EngineInitFunc)(IGameEngine *engine);
typedef void (*EngineShutdownFunc)(IGameEngine *engine);
typedef void (*EngineUpdateFunc)(IGameEngine *engine);
typedef void (*EngineRenderFunc)(IGameEngine *engine, SDL_Surface *surface);

typedef int  (*EngineGetTileFunc)(IGameEngine *engine, int x, int y);
typedef void (*EngineSetTileFunc)(IGameEngine *engine, int x, int y, int tile);
typedef void (*EngineGetViewportFunc)(IGameEngine *engine, int *x, int *y, int *w, int *h);
typedef int  (*EngineTileToWorldFunc)(IGameEngine *engine, int tile_x, int tile_y);

struct IGameEngine {
    EngineInitFunc init;
    EngineShutdownFunc shutdown;
    EngineUpdateFunc update;
    EngineRenderFunc render;

    EngineGetTileFunc get_tile;
    EngineSetTileFunc set_tile;
    EngineGetViewportFunc get_viewport;
    EngineTileToWorldFunc tile_to_world;

    void *data;
};

extern IGameEngine flipscreen_engine;
extern IGameEngine scrolling_engine;

#endif /* _ENGINE_INTERFACE_H_ */
