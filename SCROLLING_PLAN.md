# Scrolling Plan: Multi-Room Viewport and Smooth Scrolling

## Goal
Enable 2×2 (4) or 3×3 (9) room viewport with smooth pixel scrolling while preserving flip-screen mode. Current refactor already isolates per-room resources in `ROOMCONTAINER`; next step is a world-absolute `VIEWPORT` that drives cache updates, rendering offsets, and enemy activation.

---

## 0. Current State (as of 2026-08-28)

- `src/room.h:13` `ROOMCONTAINER { tiles, background, surface, shadow, room_id }` — `tiles` = `ROOM_WIDTH*ROOM_HEIGHT`, `background` = `320×136`, `surface` = `320×240`.
- `src/room.c:117` `UnpackRoom(ROOMCONTAINER*, int room)` unpacks tiles, sets `shadow`/`room_id`, and bakes background via `BuildBackgroundForContainer` (formerly `engine.c:786`).
- `src/room.h:23` `RoomContainer_Unpack(RC, room)` / `src/engine.c:776` `InitNewScreen()` uses global `game->world`.
- `TGAMEDATA` `src/engine.h:60` holds single `ROOMCONTAINER *roomcontainer` + `WORLD *world`; scalar `mode/score/mapx/mapy` etc.
- Rendering is surface-threaded: `BlitRoom(surface, RC)`, `BlitRoomOutlines(surface, RC)`, `BlitBackground(surface, RC)`, `BlitEnemies(surface)` all `SDL_Surface*` first.
- `WORLD*` param removed everywhere except `world.c`; `BlitEnemyOutlines(void)` uses `game->world`.
- Remaining single-room assumptions: `gObj_CheckTouch` uses `game->roomcontainer->tiles` only, `Ships[]` pool has no `home_room`/`world_x`.

---

## 1. Per-Room Data Model — Next

### 1.1 ROOMCONTAINER is the cache unit
Keep definition but add world-grid origin for fast offset math:

```c
// room.h
typedef struct ROOMCONTAINER {
  unsigned short *tiles;
  void *background;
  void *surface;
  unsigned int shadow;
  int room_id;      // world->room index, -1 = empty
  int grid_x, grid_y; // map coords (world->map[grid_y*mapw+grid_x]==room_id)
  int dirty;        // 1 if SetTileI modified since unpack
} ROOMCONTAINER;
```

Tile access stays `GetTileI(rc->tiles, x, y)`; future `GetTileAtWorld(wx,wy)` resolves container by `wx/320, wy/136`.

### 1.2 TGAMEDATA cache
Replace single pointer with fixed cache matching viewport coverage:

```c
// engine.h
#define VIEWPORT_W 320
#define VIEWPORT_H 136 // ACTION_SCREEN_HEIGHT
#define ROOM_PIXEL_W (ROOM_WIDTH*8)  // 320
#define ROOM_PIXEL_H (ROOM_HEIGHT*8) // 136

typedef struct VIEWPORT {
  int x, y; // world pixel top-left
  int w, h; // VIEWPORT_W, VIEWPORT_H
  int room_x0, room_y0, room_x1, room_y1; // cached visible grid rect
} VIEWPORT;

typedef struct TGAMEDATA {
  // ... scalars ...
  WORLD *world;
  VIEWPORT viewport;
  ROOMCONTAINER *cache[9]; // allocate 9, use 4 active (2×2) for room-sized viewport; 9 gives 1-room prefetch
  int cache_count; // 4 or 9
  int scroll_mode; // 0 = flip-screen, 1 = smooth
} TGAMEDATA;
```

*Why 4 vs 9?* Viewport == room size → straddling X and Y hits at most **4** rooms. 4 is sufficient and minimal. **9** (3×3) adds one-ring lookahead, avoids unpack stall on fast diagonal and simplifies neighbor checks. Cost 9×90KB ≈ 0.8MB. Recommendation: allocate 9, activate 4, keep 5 as prefetch/LRU.

Helpers:
```c
ROOMCONTAINER *GetContainerForRoom(int room_id); // scan cache by room_id
ROOMCONTAINER *GetContainerForWorld(int wx, int wy); // wx/320, wy/136 → grid → lookup
ROOMCONTAINER *AcquireContainer(int room_id, int grid_x, int grid_y); // LRU or empty slot
void UpdateRoomCache(VIEWPORT *vp); // see §2
```

---

## 2. VIEWPORT / Camera

### 2.1 World coordinates
```
WORLD_W = world->mapw * ROOM_PIXEL_W
WORLD_H = world->maph * ROOM_PIXEL_H
ship_world_x = game->mapx*ROOM_PIXEL_W + ship->x
ship_world_y = game->mapy*ROOM_PIXEL_H + ship->y
```
Ship/enemies will migrate to world-absolute `world_x/y` (see §3). Viewport is clamped:
```c
viewport.x = clamp(ship_world_x - VIEWPORT_W/2, 0, WORLD_W - VIEWPORT_W);
viewport.y = clamp(ship_world_y - VIEWPORT_H/2, 0, WORLD_H - VIEWPORT_H);
```
Flip-screen mode snaps: `viewport.x = game->mapx*ROOM_PIXEL_W; viewport.y = game->mapy*ROOM_PIXEL_H;`

### 2.2 Visible rooms & cache update
Each frame (or on ship move):
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
LRU: evict container whose `room_id` outside `(x0-1..x1+1, y0-1..y1+1)` first.

### 2.3 Rendering pipeline (offset)
For each `rc` in visible rect:
```
int dst_x = rc->grid_x*ROOM_PIXEL_W - viewport.x;
int dst_y = rc->grid_y*ROOM_PIXEL_H - viewport.y;
SDL_Rect dst = {dst_x, dst_y, ROOM_PIXEL_W, ROOM_PIXEL_H};
SDL_BlitSurface(rc->background, NULL, surface, &dst);
BlitRoomWithOffset(surface, rc, dst_x, dst_y); // loops tiles, PutTileI(surface, dst_x + x*8, dst_y + y*8)
```
Need `BlitRoomRegion` variant that clips to viewport. Same for `BlitRoomOutlines`. Background `DrawLine` already baked per-container, no per-frame offset cost.

Enemy rendering:
```
for (en = Ships) if (en->state==ACTIVE && rectIntersect(en->world_x, en->world_y, en->w, en->h, viewport))
  PutSpriteI(surface, en->world_x - viewport.x, en->world_y - viewport.y, ...)
```
Shadows via `rc->shadow`.

---

## 3. Enemy Management for Viewport

### 3.1 Global pool stays global
`Ships[32]` remains. On `InitEnemies(room_id)` compute world pos:
```c
en->home_room = room_id;
en->home_grid_x = mapx; en->home_grid_y = mapy;
en->world_x = mapx*ROOM_PIXEL_W + object->x;
en->world_y = mapy*ROOM_PIXEL_H + object->y;
en->home_rect = {home_grid_x*ROOM_PIXEL_W, home_grid_y*ROOM_PIXEL_H, ROOM_PIXEL_W, ROOM_PIXEL_H};
en->active = 0;
```
Sync `en->x = en->world_x - viewport.x` before render if needed, but keep `world_x` canonical.

### 3.2 Activation
Each frame after `UpdateRoomCache`, set `en->active = rectIntersect(en->world_x, en->world_y, ..., viewport)` OR `en->home_room` in visible rect. Only `active` enemies get `gObj_Update` for movement/AI; always update `AI_HOMING_MISSLE`/`AI_BULLET` even off-screen if desired. `gObj_CheckTouch(en->world_x, en->world_y, ...)` must resolve container by world pos.

### 3.3 Restriction to native room
Clamp `world_x/y` to `home_rect` for ground enemies (`AI_RANDOM_MOVE`, `AI_KAMIKADZE`): if `world_x < home_rect.x` or `>= home_rect.x+ROOM_PIXEL_W` then `dx=0` or bounce. Allows viewport to peek into neighbor without enemies leaking across rooms. Flying enemies may ignore.

### 3.4 Collision
Replace `GetTileI(game->roomcontainer->tiles, tx, ty)` with:
```c
RC = GetContainerForWorld(world_x, world_y);
tile = RC ? GetTileI(RC->tiles, (world_x - RC->grid_x*ROOM_PIXEL_W)/8, ...) : 1; // out of cache = solid
```
Provides correct wall collision while scrolling.

---

## 4. Data Placement & Memory

- Lazy allocate `RC->tiles/background` in `RoomContainer_Create`; free in `RoomContainer_Destroy`; on cache eviction reuse `RC` struct, `UnpackRoom` overwrites.
- `load_world`/`free_world` unchanged; `ROOM.roomIdx` already implicit via index.
- Screen metrics: `ROOM_PIXEL_W/H` as above; `WORLD_ROOM_COLS = world->mapw`.

---

## 5. Migration Checklist (from ENGINE_PLAN.md §5)

- [x] Add `ROOMCONTAINER` + `TGAMEDATA.roomcontainer` (single)
- [x] Bake background in `UnpackRoom`, remove `BuildBackground`
- [x] Surface-first `Blit*` refactor, remove `WORLD*` params
- [ ] **Add `VIEWPORT` to `TGAMEDATA`, world-absolute math, `UpdateRoomCache` (4 vs 9)**
- [ ] **Extend `TGAMEDATA` to `cache[9]` + LRU, helpers `GetContainerFor*`**
- [ ] **Migrate `Ships` to `world_x/y`, `home_room`, `home_rect`; populate in `InitEnemies`**
- [ ] **Create `BlitRoomRegion` with dst offsets; update `RenderGame` to loop over visible RCs**
- [ ] **Update `gObj_CheckTouch`/`GetTileI` to resolve container by world pos**
- [ ] **Add viewport update (smooth vs snap) in `Update_Ship`/`ChangeScreen`; toggle via `scroll_mode`**
- [ ] **Update editor `editor.c` to use viewport/cache APIs**
- [ ] **Test flip-screen then smooth scroll; verify wall collision and enemy visibility**

---

## 6. Risks (updated)

- **Tile cache thrash**: 4 is minimal, fast diagonal may unpack every frame. Mitigate with 9 + LRU.
- **Collision across rooms**: Must not use single `roomcontainer->tiles`; add world→container lookup.
- **Save/Load**: `ship_screen/mapx/mapy` now derived from `ship_world_x/y`; keep legacy fields as viewport anchor for compatibility.
- **Background seam**: baked per-RC is correct; no DrawLine offset needed at render.

---

## 7. Next Steps

1. Define `VIEWPORT`, `ROOM_PIXEL_*` in `engine.h`/`room.h`, add `viewport` + `cache[9]` to `TGAMEDATA`.
2. Implement `GetContainerFor*`, `AcquireContainer`, `UpdateRoomCache`.
3. Extend `TSHIP` with `world_x, world_y, home_room, home_rect`, convert `InitEnemies` + movement.
4. Add `BlitRoomWithOffset` and refactor `RenderGame` to multi-RC loop.
5. Hook viewport update to `Update_Ship` edge logic; keep flip mode behind flag.
6. Test with `map_zoom` editor view.

