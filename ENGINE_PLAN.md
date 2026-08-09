# Engine Plan: Per-Room Dynamic Allocation

## Goal
Group all per-room variables into a structure that can be dynamically allocated, enabling rendering of multiple rooms without spoiling data. Include separate per-room enemy object lists. The ROOM struct from world.h serves as the per-room container; TGAMEDATA holds a pointer to the current room and a flat tile array for fast rendering.

---

## 1. ROOM Struct (from world.h) — Per-Room Container

The `ROOM` struct already holds all per-room data. We keep it as the per-room container:

```c
typedef struct {
    int xs, ys;                // room dimensions in 8x8 tiles
    int pattern_num;           // number of PATTERN entries
    int object_num;            // number of OBJECT entries (enemies)
    int bg_type;               // lines or background sprite map
    int bg_num;                // number of background entries
    unsigned int background;
    unsigned int shadow;
    unsigned int line_light;
    unsigned int line_shadow;
    int procedure;             // 3 = win screen
    int bonus;                 // bonus type
    PATTERN *pattern;          // array of PATTERN entries
    OBJECT *object;            // array of enemy objects
    BGLINE *bgline;            // array of background lines
} ROOM;
```

### Allocation
All fields are already pre-allocated via `calloc` in `fread_ROOM()` (`world.c:248-253`):
```c
room->pattern = (PATTERN *)calloc(room->pattern_num, sizeof(PATTERN));
room->object = (OBJECT *)calloc(room->object_num, sizeof(OBJECT));
room->bgline = (BGLINE *)calloc(room->bg_num, sizeof(BGLINE));
```

No new structure needed — the ROOM struct already contains all per-room data.

---

## 2. TGAMEDATA — Game State with Current Room Pointer

`TGAMEDATA` already holds `WORLD *world`, `ship_screen`, `base_screen`, and game-level state. We add a pointer to the current `ROOM` and a flat tile array:

```c
typedef struct TGAMEDATA {
    int mode;
    int score;
    int ship_screen;
    int base_screen;
    int base_restart_screen;
    int lives;
    int fuel;
    int health;
    int easy_mode;
    int level;
    int screen_bonus;
    int player_attached;
    int elevator_flag;
    int hidden_level_entered;
    int ticks_for_damage;
    int garages[MAX_GARAGES][2];
    int mapx, mapy;
    int rmapx, rmapy;

    WORLD *world;
    // --- current room (derived from world->room + ship_screen) ---
    ROOM *room;
    // --- flat tile array for fast rendering ---
    unsigned short *room_tiles;
    // --- viewport (cross-room enemy tracking) ---
    Viewport viewport;
    TSHIP **viewport_enemies;
    int viewport_enemy_count;
} TGAMEDATA;
```

### `room` pointer
`game->room` is derived as `game->world->room + game->ship_screen`, giving O(1) access to the current room's data:
```c
game->room = game->world->room + game->ship_screen;
```

### `room_tiles`
An `unsigned short *` array of size `ROOM_WIDTH * ROOM_HEIGHT` (e.g., 40x25 = 1000 tiles) holds the unpacked tile data. It's allocated in `InitNewScreen()` via `UnpackRoom()` and can be reused across frames for fast rendering:
```c
room_tiles = (unsigned short *)calloc(ROOM_WIDTH * ROOM_HEIGHT, sizeof(unsigned short));
// UnpackRoom fills room_tiles
```

This avoids repeated `GetTileI()`/`SetTileI()` calls during rendering, replacing the need for the per-room tile buffer in `world.c`.

---

## 3. Viewport Concept

The viewport is the rendering window that may cross room boundaries. It holds enemies currently visible within the viewport. Enemies outside the viewport (not yet reached by the player) remain in the per-room container.

### Viewport Struct
```c
typedef struct {
    int x, y;           // viewport center (screen coordinates)
    int width, height;  // viewport dimensions in pixels
    TSHIP **enemies;    // enemies visible in viewport (dynamic)
    int enemy_count;
} Viewport;
```

### Two-Segment Enemy List

| Segment | Location | Scope |
|---|---|---|
| **Room enemies** | `ROOM.object[]` per room | Per-room; enemies that have entered the room |
| **Viewport enemies** | `Viewport.enemies[]` (dynamic, cross-room) | Per-viewport; enemies currently visible |

When the viewport moves to a new room, enemies that cross the room boundary are transferred from the old room's list to the viewport's list.

---

## 4. Key Integration Points

### Rendering (engine.c)
- `RenderGame()` → `BlitRoom()`, `BlitBackground()`, `BlitEnemies()`
- `BlitEnemies()` iterates `viewport.enemies[]` (not the global list)
- `BlitRoom()` uses `game->room` to index the ROOM struct and `game->room_tiles` for fast tile access
- `BlitRoomOutlines()` uses `game->room` to get shadow color
- Both functions must receive the current room index or container

### Viewport Enemy Management
- Viewport enemies are tracked in `viewport.enemies[]`
- When the viewport moves to a new room, enemies that are now outside the viewport are removed from the viewport list
- Enemies that cross room boundaries are added to the viewport's list
- `gObj_Update()` updates all viewport enemies each frame

### Enemy Initialization (engine.c, `InitEnemies()`)
- Currently takes `screen` (room index), gets `room->object`, creates enemies in global list
- After refactoring: takes `room_index`, iterates `world->room[room_index].object`, creates enemies in `world->room[room_index].enemies` (per-room)

### Enemy Movement (engine.c, `Update_Ship()`)
- When a ship moves into a new room, any enemies in the old room's list that are now outside the viewport are transferred to the viewport
- Enemies in the viewport list are updated per-frame by `gObj_Update()`
- Enemies in the room list are managed by `InitEnemies()` and `InitNewScreen()`

### Game State (engine.h)
- `game->ship_screen`, `game->base_screen` → `game->current_room`, `game->current_room`
- `game->screen_bonus`, `game->hidden_level_entered`, `game->elevator_flag`, `game->player_attached` → remain per-game state
- `game->world` → `game->world`
- Viewport state is part of `game`

---

## 5. TGAMEDATA Refactoring

### Static Global
`TGAMEDATA gamedata` remains static in `engine.c:61`, `game = &gamedata` is the global pointer.

### game_deinit()
```c
void game_deinit()
{
    /* Free any pre-existing world data */
    if (game->world) {
        free_world(game->world);
        game->world = NULL;
    }
}
```

### Call Sites
- `InitNewGame()` — calls `game_deinit()` at the start to clean up any pre-existing world
- `GameLoop()` — calls `game_deinit()` at the end (before exiting the main loop)

### Why not call `game_deinit()` in `LoadGame()` or `ResetGame()`?
- `LoadGame()` doesn't allocate/free world data — it copies saved state into the existing game
- `ResetGame()` calls `InitNewGame()` which already calls `game_deinit()` — avoid double-free

---

## 6. Files to Modify

1. **`src/world.h`** — Add `Viewport` struct; add `game->room` and `game->room_tiles` to `TGAMEDATA`
2. **`src/engine.h`** — Update `TGAMEDATA` fields; remove `GameRoomContainer`; add `Viewport` struct
3. **`src/engine.c`** — Update `InitEnemies()`, `RenderGame()`, `BlitEnemies()`, `BlitRoom()`, `BlitRoomOutlines()` to work with `game->room` and `game->room_tiles`
4. **`src/room.h`** — Update function signatures if needed
5. **`src/room.c`** — Update `BlitRoom()`, `BlitRoomOutlines()`, `BlitBackground()` to receive room index
6. **`src/video.c`** — May need updates for multi-room rendering
7. **`src/world.c`** — Update `load_world()`/`save_world()` if room structure changes
8. **`src/main.c`** — Update entry point for container initialization
9. **`src/input.c`** — May need updates if room-based input handling changes

---

## 7. Migration Notes

- The `Ships` global array will no longer be the sole enemy storage — per-room enemy arrays will replace it
- `screen_procedure`, `frame_skip`, `modern_background`, `title_start_flag`, `youwin_start_flag` (game-level globals) remain unchanged
- `game->world` is the main world pointer; adding per-room data to `TGAMEDATA` is safe
- `game->ship_screen` is replaced by `game->current_room` without breaking most code
- Enemy rendering logic in `BlitEnemies()` uses `viewport.enemies[]` — update to iterate per-room enemies and merge with viewport
- The viewport enemy list is the single source of truth for enemies rendered in the viewport
- `gObj_First()`/`gObj_Next()` are now used for per-room enemies only; viewport enemies are tracked separately
- The `ROOM` struct replaces the per-room structure; no new `RoomData` struct is needed
- `game->room` and `game->room_tiles` replace the per-room tile buffer and room pointer
- `game_deinit()` is added to properly clean up `game->world` before new games and at exit
- `TGAMEDATA gamedata` remains static in `engine.c` — no heap allocation for the game struct itself
