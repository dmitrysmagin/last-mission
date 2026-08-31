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
- `engine-object.h` - Object pooling extensions

### Existing Files
- `engine.c` - Main game logic (refactored)
- `engine-scrolling.c` - Scrolling implementation (to be replaced)
- `engine.h` - Engine headers
- `room.c` / `room.h` - Room management

**Note:** Only the *new* files created during refactoring use dashes. Existing source files keep their original naming.

---

## 2. Current State

### Architecture
- `ROOMCONTAINER` in `room.h:13` — `tiles` = `ROOM_WIDTH*ROOM_HEIGHT`, `background` = `320×136`, `surface` = `320×240`.
- `UnpackRoom(ROOMCONTAINER*, int room)` unpacks tiles, sets `shadow`/`room_id`, bakes background.
- `TGAMEDATA` in `engine.h:60` holds single `ROOMCONTAINER *roomcontainer` + `WORLD *world`.
- Rendering is surface-threaded: `BlitRoom(surface, RC)`, `BlitRoomOutlines(surface, RC)`, `BlitBackground(surface, RC)`, `BlitEnemies(surface)`.
- Single-room assumptions: `gObj_CheckTouch` uses `game->roomcontainer->tiles` only.

### Code Characteristics
- **24 GM_SCROLLING checks** scattered throughout `engine.c`, `object.c`, `object_enemy.c`, `object_garage.c`.
- **Conditional sprawl** in camera updates, object rendering, and collision detection.
- **Tight coupling** between game logic and rendering implementation.
- Duplicate object update logic, inconsistent coordinate handling.

### Problem Areas
- `engine.c:processGameLoop()` - Mixed mode detection and handling
- `collision.c:checkObjectCollisions()` - Mode-specific collision algorithms
- `render.c:renderScene()` - Separate rendering paths for each mode
- Difficulty adding new engine variants, testing complexity due to branching logic

---

## 3. Architecture: Strategy Pattern

### 3.1 Core Interface (`engine-interface.h`)

```c
typedef struct {
    void (*init)(void);
    void (*shutdown)(void);
    void (*update)(void);
    void (*render)(SDL_Surface *surface);
    void (*restart_level)(void);
    void (*update_respawn_from_screen)(int screen);
    int (*get_tile_at)(int x, int y);
    void (*set_tile_at)(int x, int y, int i);
    void (*get_viewport)(int *x, int *y, int *w, int *h);
    int (*tile_to_world)(int tile_x, int tile_y);
    void (*world_to_tile)(int world_x, int world_y, int *tile_x, int *tile_y);
} IGameEngine;

extern IGameEngine *active_engine;
```

### 3.2 Engine Selection (`SetGameMode`)

```c
void SetGameMode(int mode) {
    game->mode = mode;
    switch (mode) {
    case GM_SCROLLING:
        if (active_engine != &scrolling_engine) {
            if (active_engine && active_engine->shutdown) active_engine->shutdown();
            active_engine = &scrolling_engine;
            if (active_engine->init) active_engine->init();
        }
        break;
    case GM_GAME:
    case GM_DEMO:
        if (active_engine != &flipscreen_engine) {
            if (active_engine && active_engine->shutdown) active_engine->shutdown();
            active_engine = &flipscreen_engine;
            if (active_engine->init) active_engine->init();
        }
        break;
    // ...
    }
}
```

### 3.3 Game Loop Integration

```c
void DoGame() {
    switch(game->mode) {
    case GM_GAME:
        // ... existing flip-screen logic ...
        break;
    case GM_SCROLLING:
        if (active_engine && active_engine->update) active_engine->update();
        if (active_engine && active_engine->render) active_engine->render(game->roomcontainer->surface);
        break;
    }
}
```

### 3.4 Benefits

| Aspect | Current (Conditional) | Proposed (Strategy) |
|--------|----------------------|---------------------|
| **Lines of Code** | ~1200 (with duplication) | ~600 (separated) |
| **GM_SCROLLING Checks** | 24 scattered | 0 in logic code |
| **New Engine Addition** | Modify all conditionals | Single new class |
| **Test Complexity** | High (branch coverage) | Low (isolated tests) |

---

## 4. Per-Room Data Model

### 4.1 ROOMCONTAINER is the cache unit

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

### 4.2 TGAMEDATA cache

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

typedef struct TGAMEDATA {
    // ... existing scalars ...
    WORLD *world;
    VIEWPORT viewport;
    ROOMCONTAINER *cache[9];
    int cache_count;
} TGAMEDATA;
```

### 4.3 Tile Access

```c
ROOMCONTAINER *GetContainerForRoom(int room_id);
ROOMCONTAINER *GetContainerForWorld(int wx, int wy);
ROOMCONTAINER *AcquireContainer(int room_id, int grid_x, int grid_y);
void UpdateRoomCache(VIEWPORT *vp);
```

---

## 5. Camera / Viewport Abstraction

### 5.1 World coordinates

```
WORLD_W = world->mapw * ROOM_PIXEL_W
WORLD_H = world->maph * ROOM_PIXEL_H
ship_world_x = game->mapx*ROOM_PIXEL_W + ship->x
ship_world_y = game->mapy*ROOM_PIXEL_H + ship->y
```

Viewport clamped:
```c
viewport.x = clamp(ship_world_x - VIEWPORT_W/2, 0, WORLD_W - VIEWPORT_W);
viewport.y = clamp(ship_world_y - VIEWPORT_H/2, 0, WORLD_H - VIEWPORT_H);
```

### 5.2 Visible rooms & cache update

```c
void UpdateRoomCache(VIEWPORT *vp) {
    int x0 = vp->x / ROOM_PIXEL_W, y0 = vp->y / ROOM_PIXEL_H;
    int x1 = (vp->x+vp->w-1)/ROOM_PIXEL_W, y1 = (vp->y+vp->h-1)/ROOM_PIXEL_H;
    vp->room_x0=x0; vp->room_y0=y0; vp->room_x1=x1; vp->room_y1=y1;
    for (y=y0..y1) for (x=x0..x1) {
        int room_id = world->map[y*mapw+x]; if (!room_id) continue;
        RC = GetContainerForRoom(room_id);
        if (!RC || RC->dirty) { RC = AcquireContainer(room_id,x,y); RoomContainer_Unpack(RC, room_id); }
    }
}
```

### 5.3 Scroll Modes

- **Flip-Screen**: Camera snaps to room boundaries. `viewport.x = game->mapx*ROOM_PIXEL_W`.
- **Smooth Scrolling**: Camera moves in pixel increments, clamped to world bounds. Enemies use world-absolute positions.

### 5.4 Rendering Pipeline

For each `rc` in visible rect:
```c
int dst_x = rc->grid_x*ROOM_PIXEL_W - viewport.x;
int dst_y = rc->grid_y*ROOM_PIXEL_H - viewport.y;
SDL_Rect dst = {dst_x, dst_y, ROOM_PIXEL_W, ROOM_PIXEL_H};
SDL_BlitSurface(rc->background, NULL, surface, &dst);
BlitRoomWithOffset(surface, rc, dst_x, dst_y);
```

Enemy rendering:
```c
for (en = Ships) if (en->state==ACTIVE && rectIntersect(en->world_x, en->world_y, en->w, en->h, viewport))
    PutSpriteI(surface, en->world_x - viewport.x, en->world_y - viewport.y, ...)
```

---

## 6. Coordinate System Abstraction

### 6.1 Coordinate Systems

| System | Description | Range |
|--------|-------------|-------|
| **World** | Continuous space across entire map | 0 to MAP_WIDTH * 16, MAP_HEIGHT * 16 |
| **Screen** | Relative to camera viewport | 0 to SCREEN_WIDTH, SCREEN_HEIGHT |
| **Room** | 16x16 tile rooms (flip-screen only) | 0-15 rooms in each dimension |

### 6.2 Conversion Matrix

| From\To | World | Screen | Room |
|---------|-------|--------|------|
| World   | - | `worldToScreen()` | `worldToRoom()` |
| Screen  | `screenToWorld()` | - | `screenToRoom()` |
| Room    | `roomToWorld()` | `roomToScreen()` | - |

### 6.3 Coordinate Conversion Helpers

```c
// engine-coordinates.h
int worldToScreenX(int worldX, IGameEngine* engine);
int worldToScreenY(int worldY, IGameEngine* engine);
int screenToWorldX(int screenX, IGameEngine* engine);
int screenToWorldY(int screenY, IGameEngine* engine);
int object_getScreenX(Object* obj, IGameEngine* engine);
int object_getScreenY(Object* obj, IGameEngine* engine);
```

---

## 7. Collision System

### 7.1 Collision Interface

```c
typedef struct {
    int (*check_tile_collision)(int x, int y, TSHIP *gobj);
    int (*check_object_collision)(int x1, int y1, TSHIP *gobj1, int x2, int y2, TSHIP *gobj2);
    int (*check_solid_collision)(int x, int y, TSHIP *gobj);
    int (*get_tile_flags)(int tile);
    void (*mark_tile_destroyed)(int wx, int wy);
    int (*is_tile_destroyed)(int wx, int wy);
} ICollisionSystem;
```

### 7.2 Implementation

- **Flip-Screen**: Check against fixed 16x16 room boundaries, simplified collision grid.
- **Scrolling**: Continuous world boundaries, check against map edges, partial room overlaps handled.

### 7.3 Object Collision

`gObj_CheckTouch` checks:
1. Bounds (screen bounds for flip, room rect for scrolling)
2. Object overlap via `gObj_CheckOverlap` / `gObj_CheckDestruction`
3. Tile collision via `GetTileI` / `Scrolling_GetTileAtWorld`

---

## 8. Enemy Management

### 8.1 Per-Room Enemy Lists

```c
typedef struct {
    int count;
    TSHIP **enemies;
} RoomEnemies;
```

- Populate during `InitEnemies(screen)`: each enemy carries `roomIdx` for spatial queries.

### 8.2 Activation

Each frame after `UpdateRoomCache`:
- Set `en->active = rectIntersect(en->world_x, en->world_y, ..., viewport)`
- Only `active` enemies get `gObj_Update` for movement/AI.

### 8.3 Restriction to Native Room

Clamp `world_x/y` to `home_rect` for ground enemies. Allows viewport to peek into neighbor without enemies leaking across rooms.

### 8.4 AI Differences

| Feature | Flip-Screen | Scroller |
|---------|-------------|----------|
| Detection Range | Per-room | Line-of-sight |
| Tracking | Simple | Predictive |
| Pathfinding | Room grid | Continuous |
| State Persistence | Room-local | Global |

---

## 9. Migration Path

### Phase 1: Interface Definition ✓
- [x] Create `engine-interface.h` with `IGameEngine` typedef
- [x] Create `engine-flipscreen.c` with flip-screen implementation
- [x] Create `engine-scroller.c` with scrolling implementation
- [x] Create `engine-coordinates.h/.c` for coordinate abstraction
- [x] Create `engine-collision.h/.c` for collision abstraction
- [x] Add `active_engine` pointer to `TGAMEDATA`
- [x] Replace 24 `GM_SCROLLING` checks with `IS_SCROLLING()` macro

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
- [ ] Remove old `engine-scrolling.c`
- [ ] Remove remaining `GM_SCROLLING` checks
- [ ] Update editor to use viewport/cache APIs
- [ ] Test both modes with existing levels

---

## 10. Risks

- **Tile cache thrash**: 4 rooms is minimal, fast diagonal may unpack every frame. Mitigate with 9 + LRU.
- **Collision across rooms**: Must not use single `roomcontainer->tiles`; add world→container lookup.
- **Save/Load**: `ship_screen/mapx/mapy` now derived from `ship_world_x/y`; keep legacy fields for compatibility.
- **Background seam**: Baked per-RC is correct; no DrawLine offset needed at render.
- **Virtual function call overhead**: Negligible (~4 bytes pointer dereference).
- **Memory overhead**: Strategy object ~32 bytes per game state; tradeoff for maintainability.

---

*Document version: 2.0 — Combined from SCROLLING_PLAN, ENGINE_REFACTOR, ENGINE_PLAN*
