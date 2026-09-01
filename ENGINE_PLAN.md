# Engine Architecture: Flip-Screen & Scrolling Engines

## Goal

Complete separation of the flip-screen and scrolling engines into a Strategy Pattern architecture, enabling clean multi-room viewport rendering with smooth pixel scrolling while preserving the original flip-screen mode.

---

## 1. File Naming Convention

All new engine-related files use **dash naming convention** (not underscore):

### Core Engine Files
- `engine-interface.h` - Strategy interface definition
- `engine-flipscreen.c` - Flip-screen engine implementation
- `engine-scroller.c` - Scrolling engine implementation

### Helper Module Files
- `engine-coordinates.h` / `engine-coordinates.c` - Coordinate system abstraction
- `engine-coordinates-scrolling.h` / `engine-coordinates-scrolling.c` - Scrolling coordinate helpers
- `engine-collision.h` / `engine-collision.c` - Collision system abstraction

### Existing Files
- `engine.c` - Main game logic (refactored)
- `engine-scrolling.c` - Scrolling implementation (kept as delegate for scroller engine)
- `engine.h` - Engine headers
- `room.c` / `room.h` - Room management

**Note:** Only the *new* files created during refactoring use dashes. Existing source files keep their original naming.

---

## 2. Current State

### Architecture
- `ROOMCONTAINER` in `room.h:13` — `tiles` = `ROOM_WIDTH*ROOM_HEIGHT`, `background` = `320×136`, `surface` = `320×240`.
- `UnpackRoom(ROOMCONTAINER*, int room)` unpacks tiles, sets `shadow`/`room_id`, bakes background.
- `TGAMEDATA` in `engine.h` holds `IGameEngine *engine` pointer + single `ROOMCONTAINER *roomcontainer` + `WORLD *world`.
- Rendering is surface-threaded: `BlitRoom(surface, RC)`, `BlitRoomOutlines(surface, RC)`, `BlitBackground(surface, RC)`, `BlitEnemies(surface)`.
- `IS_SCROLLING_ENGINE()` macro replaces `GM_SCROLLING` checks in game logic.

### Code Characteristics
- **Strategy Pattern** with `IGameEngine` vtable for engine dispatch
- **Unified coordinate system** via `engine-coordinates.c`
- **Collision abstraction** via `engine-collision.c` with `gObj_CheckTouch` and `gObj_CheckDestruction`
- Engine switching via `SetEngineMode()` function

---

## 3. Architecture: Strategy Pattern

### 3.1 Core Interface (`engine-interface.h`)

```c
typedef struct IGameEngine IGameEngine;

struct IGameEngine {
    void (*init)(IGameEngine *engine);
    void (*shutdown)(IGameEngine *engine);
    void (*update)(IGameEngine *engine);
    void (*render)(IGameEngine *engine, SDL_Surface *surface);
    int (*get_tile)(IGameEngine *engine, int x, int y);
    void (*set_tile)(IGameEngine *engine, int x, int y, int tile);
    void (*get_viewport)(IGameEngine *engine, int *x, int *y, int *w, int *h);
    int (*tile_to_world)(IGameEngine *engine, int tile_x, int tile_y);
    void *data;
};

extern IGameEngine flipscreen_engine;
extern IGameEngine scrolling_engine;
```

### 3.2 Engine Selection (`SetEngineMode`)

```c
void SetEngineMode(int mode) {
    if (game->engine) {
        if (game->engine->shutdown)
            game->engine->shutdown(game->engine);
    }

    switch (mode) {
    case GM_SCROLLING:
        game->engine = &scrolling_engine;
        break;
    case GM_GAME:
    case GM_DEMO:
    default:
        game->engine = &flipscreen_engine;
        break;
    }

    if (game->engine) {
        if (game->engine->init)
            game->engine->init(game->engine);
    }
}
```

### 3.3 Game State Integration

```c
// engine.h
typedef struct TGAMEDATA {
    int mode;
    // ... other fields ...
    WORLD *world;
    ROOMCONTAINER *roomcontainer;
    IGameEngine *engine;  // Active engine interface
} TGAMEDATA;
```

### 3.4 Benefits

| Aspect | Before (Conditional) | After (Strategy) |
|--------|---------------------|------------------|
| **GM_SCROLLING Checks** | 24 scattered | 0 in logic code |
| **New Engine Addition** | Modify all conditionals | Single new class |
| **Test Complexity** | High (branch coverage) | Low (isolated tests) |
| **Code Maintainability** | Tight coupling | Separated concerns |

---

## 4. Coordinate System Abstraction

### 4.1 Coordinate Systems

| System | Description | Range |
|--------|-------------|-------|
| **World** | Continuous space across entire map | 0 to MAP_WIDTH * 16, MAP_HEIGHT * 16 |
| **Screen** | Relative to camera viewport | 0 to SCREEN_WIDTH, SCREEN_HEIGHT |
| **Room** | 16x16 tile rooms (flip-screen only) | 0-15 rooms in each dimension |

### 4.2 Conversion Functions

```c
// engine-coordinates.h
int Coords_WorldToScreenX(int worldX, IGameEngine *engine);
int Coords_WorldToScreenY(int worldY, IGameEngine *engine);
int Coords_ScreenToWorldX(int screenX, IGameEngine *engine);
int Coords_ScreenToWorldY(int screenY, IGameEngine *engine);
int Coords_ObjectGetScreenX(TSHIP *obj, IGameEngine *engine);
int Coords_ObjectGetScreenY(TSHIP *obj, IGameEngine *engine);
int Coords_ObjectInViewport(TSHIP *obj, IGameEngine *engine);
```

### 4.3 Helper Macro

```c
// object.h
#define IS_SCROLLING_ENGINE() (game && game->engine == &scrolling_engine)
```

---

## 5. Collision System

### 5.1 Collision Interface

```c
// engine-collision.h
int Collision_CheckTile(int x, int y, TSHIP *gobj);
int Collision_CheckOverlap(int x1, int y1, TSHIP *gobj1, int x2, int y2, TSHIP *gobj2);
int Collision_CheckSolid(int x, int y, TSHIP *gobj);
int Collision_CheckBounds(int x, int y, int w, int h, IGameEngine *engine);
int Collision_CheckObjectBounds(TSHIP *obj, IGameEngine *engine);
```

### 5.2 Implementation

- **Flip-Screen**: Check against fixed 16x16 room boundaries, simplified collision grid.
- **Scrolling**: Continuous world boundaries, check against map edges, partial room overlaps handled.

### 5.3 Object Collision

`gObj_CheckTouch` checks:
1. Bounds (screen bounds for flip, room rect for scrolling)
2. Object overlap via `gObj_CheckOverlap` / `gObj_CheckDestruction`
3. Tile collision via `GetTileI` / `Scrolling_GetTileAtWorld`

---

## 6. Migration Path

### Phase 1: Interface Definition ✓
- [x] Create `engine-interface.h` with `IGameEngine` typedef
- [x] Create `engine-flipscreen.c` with flip-screen implementation
- [x] Create `engine-scroller.c` with scrolling implementation
- [x] Create `engine-coordinates.h/.c` for coordinate abstraction
- [x] Create `engine-collision.h/.c` for collision abstraction
- [x] Add `IGameEngine *engine` field to `TGAMEDATA`
- [x] Implement `SetEngineMode()` function
- [x] Replace 24 `GM_SCROLLING` checks with `IS_SCROLLING_ENGINE()` macro

### Phase 2: Data Model
- [ ] Add `VIEWPORT`, `ROOM_PIXEL_*` to `engine.h`/`room.h`
- [ ] Add `viewport` + `cache[9]` to `TGAMEDATA`
- [ ] Implement `GetContainerFor*`, `AcquireContainer`, `UpdateRoomCache`
- [ ] Remove single `room_tiles` global

### Phase 3: Enemy Migration
- [ ] Extend `TSHIP` with `world_x`, `world_y`, `home_room`, `home_rect`
- [ ] Convert `InitEnemies` to populate per-room enemy lists
- [ ] Migrate `Ships` pool to `world_x/y`, `home_room`, `home_rect`

### Phase 4: Rendering Pipeline
- [ ] Create `BlitRoomRegion` with dst offsets
- [ ] Update `RenderGame` to loop over visible RCs
- [ ] Add viewport update (smooth vs snap) in `Update_Ship`/`ChangeScreen`

### Phase 5: Cleanup
- [ ] Remove old `engine-scrolling.c` (currently kept as delegate)
- [ ] Update editor to use viewport/cache APIs
- [ ] Test both modes with existing levels

---

## 7. Scrolling Engine Implementation Details

### 7.1 Per-Room Data Model

`ROOMCONTAINER` is the cache unit:
```c
typedef struct ROOMCONTAINER {
    unsigned short *tiles;
    void *background;
    void *surface;
    unsigned int shadow;
    int room_id;
    int grid_x, grid_y;
    int dirty;
} ROOMCONTAINER;
```

### 7.2 Viewport & Camera

```c
#define VIEWPORT_W 320
#define VIEWPORT_H 136
#define ROOM_PIXEL_W (ROOM_WIDTH*8)   // 320
#define ROOM_PIXEL_H (ROOM_HEIGHT*8)  // 136

typedef struct VIEWPORT {
    int x, y;
    int w, h;
    int room_x0, room_y0, room_x1, room_y1;
} VIEWPORT;
```

Viewport clamped:
```c
viewport.x = clamp(ship_world_x - VIEWPORT_W/2, 0, WORLD_W - VIEWPORT_W);
viewport.y = clamp(ship_world_y - VIEWPORT_H/2, 0, WORLD_H - VIEWPORT_H);
```

### 7.3 Enemy Management

- Enemies use world-absolute positions (`world_x`, `world_y`)
- `home_room`, `home_gx`, `home_gy` track native room
- `scroll_active` flag for viewport activation
- Clamped to `home_rect` for ground enemies

### 7.4 Cache Update

```c
void UpdateRoomCache(VIEWPORT *vp) {
    int x0 = vp->x / ROOM_PIXEL_W, y0 = vp->y / ROOM_PIXEL_H;
    int x1 = (vp->x+vp->w-1)/ROOM_PIXEL_W, y1 = (vp->y+vp->h-1)/ROOM_PIXEL_H;
    for (y=y0..y1) for (x=x0..x1) {
        int room_id = world->map[y*mapw+x]; if (!room_id) continue;
        RC = GetContainerForRoom(room_id);
        if (!RC || RC->dirty) { RC = AcquireContainer(room_id,x,y); RoomContainer_Unpack(RC, room_id); }
    }
}
```

---

## 8. Risks

- **Tile cache thrash**: 4 rooms is minimal, fast diagonal may unpack every frame. Mitigate with 9 + LRU.
- **Collision across rooms**: Must not use single `roomcontainer->tiles`; add world→container lookup.
- **Save/Load**: `ship_screen/mapx/mapy` now derived from `ship_world_x/y`; keep legacy fields for compatibility.
- **Background seam**: Baked per-RC is correct; no DrawLine offset needed at render.
- **Virtual function call overhead**: Negligible (~4 bytes pointer dereference).
- **Memory overhead**: Strategy object ~32 bytes per game state; tradeoff for maintainability.

---

*Document version: 3.0 — Merged from SCROLLING_PLAN.md and ENGINE_REFACTOR.md*
*Last updated: All Phase 1 tasks completed*
