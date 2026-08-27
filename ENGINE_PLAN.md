# Engine Plan: Multi-Room Viewport and Smooth Scrolling

## Goal
Enable rendering of multiple rooms simultaneously and allow the viewport to scroll smoothly across room boundaries while preserving the original flip-screen mode. Refactor data placement so each room’s resources are self-contained, then introduce a camera/viewport abstraction that samples the surrounding room grid for tiles, enemies, and backgrounds.

---

## 1. Per-Room Data Model

### Existing Structures
- `ROOM` in `world.h` already groups tile patterns, enemy objects, and background lines per room.
- `TGAMEDATA` in `engine.h` currently holds a single `room_tiles` array and a `world` pointer.

### Proposed Changes
1. **Room Store**
   - Add `RoomStore` to `world.h`:
     ```c
     typedef struct {
       int room_num;
       ROOM *rooms; // from world->room
     } RoomStore;
     ```
   - Expose helpers `RoomStore_Get(RoomStore*, int idx)` and `RoomStore_InBounds(RoomStore*, int x, int y)`.

2. **TGAMEDATA Refactoring**
   - Replace single `unsigned short room_tiles[ROOM_WIDTH * ROOM_HEIGHT]` with:
     ```c
     typedef struct {
       int x, y; // room grid coordinates
       ROOM *room;
       unsigned short *tiles; // dynamically allocated ROOM_WIDTH*ROOM_HEIGHT
       int dirty; // 1 if tile data changed since last render
     } RoomTileCache;

     typedef struct TGAMEDATA {
       // ... existing scalar fields ...
       WORLD *world;
       RoomTileCache room_caches[9]; // 3x3 surrounding current room
       // camera state
       int cam_x, cam_y; // world pixel position
       int mode; // GM_GAME, GM_EDITOR, etc.
     } TGAMEDATA;
     ```
   - `room_caches` store pre-unpacked tile buffers for the current room and its eight neighbors.

3. **Tile Access**
   - Introduce `GetRoomTiles(TGAMEDATA*, int gridX, int gridY)` that returns the cached `unsigned short*` for that room (or NULL if out of bounds).
   - Update all `GetTileI`/`SetTileI` call sites to use `GetRoomTiles()` to resolve the correct buffer.

---

## 2. Camera / Viewport Abstraction

### 2.1 Camera Struct
```c
typedef struct {
  int x, y;        // world pixel position (top-left of screen)
  int width, height; // viewport size (equal to screen size unless editor mode)
} Camera;
```
- Camera is stored in `TGAMEDATA`.
- On each frame, compute which rooms intersect the camera rectangle using simple integer division:
  ```c
  int roomX0 = cam.x / ROOM_PIXEL_WIDTH;
  int roomY0 = cam.y / ROOM_PIXEL_HEIGHT;
  int roomX1 = (cam.x + cam.width - 1) / ROOM_PIXEL_WIDTH;
  int roomY1 = (cam.y + cam.height - 1) / ROOM_PIXEL_HEIGHT;
  ```

### 2.2 Rendering Pipeline
1. **Determine Visible Rooms**
   - Compute `(roomX0,roomY0)-(roomX1,roomY1)` rectangle.
   - Ensure indices are clamped to world bounds.

2. **Unpack Tiles**
   - For each room in the rectangle, ensure `RoomTileCache` is populated (call `UnpackRoom` if `dirty` or cache empty).
   - Mark cache as clean.

3. **Background Rendering**
   - For each visible room, call `BlitBackground` offset by `(roomIdxX * ROOM_PIXEL_WIDTH - cam.x, roomIdxY * ROOM_PIXEL_HEIGHT - cam.y)`.
   - Adjust `DrawLine` calls accordingly.

4. **Tile Rendering**
   - For each visible room, compute source rectangle of tiles that intersect camera.
   - Use `BlitRoomRegion(room_tiles, srcX, srcY, width, height, destX, destY)` to blit only the needed portion.

5. **Enemy Rendering**
   - Enemies are still stored in a global pool but each enemy belongs to a specific room (store `roomIdx` in `TSHIP`).
   - Before rendering, iterate the pool, check if enemy’s bounding box intersects camera, compute screen coordinates:
     ```c
     int sx = enemy->x - cam.x;
     int sy = enemy->y - cam.y;
     ```
   - Only blit if within viewport.

6. **Outlines / Shadows**
   - Same as above: iterate enemies, compute screen position relative to camera.

### 2.3 Scroll Modes
- **Flip-Screen (Legacy)**
  - Camera snaps to room boundaries (camera position always a multiple of `ROOM_PIXEL_WIDTH`/`HEIGHT`).
  - Triggered by directional input at screen edge.
  - Keeps original behavior; code path can be toggled via `TGAMEDATA.mode`.

- **Smooth Scrolling (New)**
  - Camera moves in pixel increments, clamped to world bounds.
  - When camera crosses a room boundary, new rooms become visible; existing `room_caches` handle tile data.
  - Enemies remain in pool; their positions are world-relative.

---

## 3. Enemy Management for Multi-Room

### 3.1 Per-Room Enemy Lists
- Extend `ROOM` (or a new `RoomEnemies` struct) with:
  ```c
  typedef struct {
    int count;
    TSHIP **enemies; // dynamic array of pointers to Ships[]
  } RoomEnemies;
  ```
- Populate during `InitEnemies(screen)`: for each enemy created, add pointer to the room’s list and store `roomIdx` in the enemy struct.

### 3.2 Viewport Enemy Selection
- On each frame, build a temporary list of enemies whose bounding boxes intersect the camera:
  1. For each visible room, iterate its `RoomEnemies.enemies`.
  2. Compute screen position; if visible, push to `viewport_enemies` array.
  3. Use this list for updates and rendering.

### 3.3 Enemy Updates
- Continue to call `gObj_Update` on all active enemies (even off-screen) to preserve game logic (e.g., homing missiles).
- Alternatively, defer updates for off-screen enemies if performance becomes an issue.

---

## 4. Data Placement Refactoring

### 4.1 Remove Static Globals
- `room_tiles` in `engine.c` is removed; replaced by `TGAMEDATA.room_caches`.
- `Ships[]` remains global but each enemy carries `roomIdx` for spatial queries.

### 4.2 Room Data Loading
- Keep existing `load_world`/`free_world` functions.
- Add `RoomStore` to expose `world->room` array and helper functions.
- Ensure `ROOM` contains `roomIdx` (its index) for fast reverse lookup.

### 4.3 Memory Management
- `RoomTileCache.tiles` allocated lazily when room becomes visible (or pre-allocated for the 3x3 grid).
- Free tile caches on level change or game deinit.
- Maintain a small pool of `TSHIP*` per room to avoid per-frame scanning of entire `Ships[]`.

### 4.4 Screen Metrics
- Define constants:
  ```c
  #define ROOM_PIXEL_WIDTH  (ROOM_WIDTH * 8)   // 320
  #define ROOM_PIXEL_HEIGHT (ROOM_HEIGHT * 8)  // 200
  #define WORLD_ROOM_COLS   (world->mapw)
  #define WORLD_ROOM_ROWS   (world->maph)
  ```

---

## 5. Migration Checklist

1. **Add structs** (`RoomStore`, `RoomTileCache`, `Camera`, `RoomEnemies`) to `world.h` / `engine.h`.
2. **Update TGAMEDATA** to hold camera, tile caches, per-room enemy lists.
3. **Refactor `InitEnemies`** to populate per-room enemy lists and assign `roomIdx`.
4. **Implement `GetRoomTiles`** and replace all `game->room_tiles` usages.
5. **Create rendering helpers** (`BlitRoomRegion`, `BlitBackgroundOffset`) that accept camera offsets.
6. **Update `RenderGame`** to compute visible rooms, blit backgrounds, tiles, shadows, and enemies relative to camera.
7. **Add camera update logic** (smooth scroll or snap) and integrate with `Update_Ship` edge detection.
8. **Refactor `ChangeScreen`** to update camera position and room caches when crossing boundaries.
9. **Ensure legacy flip-screen mode** remains functional via a mode flag.
10. **Update editor** (`editor.c`) to use new camera/tile APIs (optional but recommended).
11. **Test** both flip-screen and smooth scrolling with existing levels; verify enemy visibility and collisions.

---

## 6. Potential Risks

- **Collision Detection**: `gObj_CheckTouch` uses tile queries; must ensure the correct tile cache is used based on the enemy’s world position.
- **Performance**: Unpacking tiles for up to 9 rooms each frame is acceptable (rooms are small), but tile blitting must be clipped to the viewport.
- **Save/Load**: Save files store `ship_screen`/`mapx`/`mapy`; these remain valid but now represent the camera’s anchor room rather than a single visible room.
- **Background Alignment**: `BlitBackground` draws full-screen backgrounds; need to offset per room to avoid misalignment when multiple rooms are visible.

---

## 7. Next Steps

1. Define constants and helper functions in `world.h`/`engine.h`.
2. Implement `RoomTileCache` allocation/cleanup and `GetRoomTiles`.
3. Extend `ROOM` or add `RoomEnemies` for per-room enemy tracking.
4. Modify `InitEnemies` to populate room enemy lists.
5. Build camera update routines and integrate with `Update_Ship`.
6. Refactor rendering pipeline to use camera offsets and multi-room blitting.
7. Test flip-screen mode to ensure no regressions.
8. Implement smooth scrolling and verify enemy visibility and collisions.
