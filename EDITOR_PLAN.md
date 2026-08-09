# Editor Improvement Plan

## 1. Map Edit — Show Room Number Over Each Cell

### Current State
`ShowMap()` only renders rooms where `roomnum != 0`. Void cells are skipped. No text overlay.

### Implementation
- After the existing render loop in `ShowMap()`, iterate the same grid again
- For each cell, compute the room number via `getscreen()`
- Render text centered in each cell using `PutString()`
- For void cells (roomnum == 0), display `" -"`
- Text position: `room_w * x + room_w/2 - 12`, `room_h * y + room_h/2 - 4`

### Functions Available
- `PutString(int x, int y, char *p)` — `sprites.h:26`

### Considerations
- At zoom level -2 (46x15 px cells), text may be hard to read
- Should only render text when `room_w >= 24 && room_h >= 16` (zoom levels -1 and higher)
- Or render a simplified single-char indicator at small zoom levels

---

## 2. Map Edit — Show Map Borders with Colored Rectangle

### Current State
Only a red cursor rectangle is drawn: `DrawRect(room_w * cur_x, room_h * cur_y, room_w, room_h, RGB(255, 0, 0))`.

### Implementation Options

**Option A — Full map border (complex):**
- Render the entire map scaled to fit the screen
- Requires different rendering approach than current cursor-centered view

**Option B — Viewport border (simpler, recommended):**
- Draw the viewport boundary with `RGB(0, 255, 0)` (green) to distinguish from red cursor
- Add info text showing map dimensions `mapw x maph` and visible range

### Implementation (Option B)
```c
// Draw viewport border
DrawRect(0, 0, SCREEN_WIDTH, ACTION_SCREEN_HEIGHT, RGB(0, 255, 0));
// Draw cursor on top
DrawRect(room_w * cur_x, room_h * cur_y, room_w, room_h, RGB(255, 0, 0));
// Show map dimensions in info panel
sprintf(string, "MAP %02ix%02i", game->world->mapw, game->world->maph);
```

---

## 3. Map Edit — Navigate to Void Entry and Insert Existing Room

### Current State
- `MapEdit()` navigates freely but ENTER on a void cell sets `cur_room = 0` (invalid)
- No `setscreen()` function exists — map is read-only from editor

### Implementation

#### a) Create `setscreen()` function
**Location:** `engine.c` / `engine.h`

```c
void setscreen(int mapx, int mapy, int roomnum) {
    if (mapx < 0 || mapx >= game->world->mapw) return;
    if (mapy < 0 || mapy >= game->world->maph) return;
    game->map[mapy * game->world->mapw + mapx] = roomnum;
}
```

#### b) Add room picker in MapEdit
- Add `new_room_selection` variable to editor state (initially 1)
- When cursor is on a void cell (`getscreen() == 0`):
  - Display "PLACE ROOM: XXX" in info panel
  - Use `SC_Z`/`SC_X` to cycle through available room numbers (1 to `room_num - 1`)
  - Press `SC_SPACE` or `SC_ENTER` to write it to the map via `setscreen()`
- When cursor is on existing room, ENTER enters RoomEdit mode (current behavior)

#### Key Mapping
| Key | Action |
|-----|--------|
| Z/X | Cycle room number to place (when on void cell) |
| ENTER | Enter RoomEdit for current room (existing) or place selected room (on void) |
| Arrow keys | Navigate map |

---

## 4. Room Edit — Extend/Shrink Room List (Add/Delete Rooms)

### Current State
- `game->world->room` is fixed array allocated by `calloc(world->room_num, sizeof(ROOM))`
- `room_num` is the count and is used everywhere as array size
- No dynamic resizing exists

### Implementation

#### a) Add room (append to end)
**Trigger:** New key in MapEdit or RoomEdit (e.g., `SC_A` for "add")

```c
game->world->room_num++;
game->world->room = realloc(game->world->room, game->world->room_num * sizeof(ROOM));
ROOM *new_room = &game->world->room[game->world->room_num - 1];
memset(new_room, 0, sizeof(ROOM));
// Initialize with defaults
new_room->xs = 40;
new_room->ys = 17;
new_room->pattern_num = 0;
new_room->object_num = 0;
new_room->bg_num = 0;
new_room->bg_type = 0;
new_room->background = 0;
new_room->shadow = 0;
new_room->line_light = 0;
new_room->line_shadow = 0;
new_room->procedure = 0;
new_room->bonus = 0;
new_room->pattern = NULL;
new_room->object = NULL;
new_room->bgline = NULL;
```

#### b) Delete last room
**Trigger:** New key (e.g., `SC_Q` for "delete last")

```c
if (game->world->room_num > 1) {
    ROOM *last = &game->world->room[game->world->room_num - 1];
    free(last->pattern);
    free(last->object);
    free(last->bgline);
    // Remove from map references
    for (int i = 0; i < game->world->mapw * game->world->maph; i++) {
        if (game->world->map[i] == game->world->room_num - 1)
            game->world->map[i] = 0;
    }
    game->world->room_num--;
    game->world->room = realloc(game->world->room, game->world->room_num * sizeof(ROOM));
}
```

### Key Concerns
- Adding a room at the end doesn't shift existing indices (safe)
- Deleting the **last** room is safe — no higher indices exist
- Deleting a **middle** room would require renumbering all map references (complex, avoid for now)
- All code using `room_num` as loop bound will automatically adapt

---

## 5. Room Edit — Create New List of Objects, Place Them

### Current State
- Objects stored in `ROOM.object` array of `OBJECT` structs
- Editor has no object placement UI at all
- No way to add/remove/move objects

### Implementation

#### a) Object placement mode
**Toggle:** `SC_TAB` in RoomEdit to switch between Pattern Mode and Object Mode

State variables needed:
```c
static int object_mode = 0;      // 0 = pattern edit, 1 = object edit
static int cur_object = 0;       // current object index
static int new_object_type = 0;  // object type to place
static int obj_cursor_x = 0;     // object cursor position (pixels)
static int obj_cursor_y = 0;
```

#### b) Object mode controls
| Key | Action |
|-----|--------|
| Arrow keys | Move object cursor |
| Z/X | Cycle object type/index |
| SPACE | Place object at cursor |
| DELETE | Remove last placed object |
| TAB | Toggle pattern/object mode |

#### c) Adding an object
```c
room->object_num++;
room->object = realloc(room->object, room->object_num * sizeof(OBJECT));
OBJECT *obj = &room->object[room->object_num - 1];
obj->x = obj_cursor_x;
obj->y = obj_cursor_y;
obj->index = new_object_type;
obj->speed = 0;
obj->minframe = 0;
obj->maxframe = 0;
obj->ai = 0;
obj->garage_id = 0;
```

#### d) Removing an object
```c
if (room->object_num > 0) {
    room->object_num--;
    room->object = realloc(room->object, room->object_num * sizeof(OBJECT));
    // Order doesn't matter, just drop last
}
```

#### e) Rendering objects in editor
- Draw colored rectangles at object positions
- Highlight selected object with brighter color
- Show object count in info panel

### OBJECT Structure (from world.h:33)
```c
typedef struct {
    int x, y;           // pixel position
    int index;          // sprite/type index
    int speed;          // movement speed
    int minframe, maxframe; // animation frames
    int ai;             // AI behavior type
    int garage_id;      // garage association
} OBJECT;
```

---

## New Functions Summary

| Function | Location | Purpose |
|----------|----------|---------|
| `setscreen(mapx, mapy, roomnum)` | `engine.c` / `engine.h` | Write room number to map |
| Room append/delete | `editor.c` | Dynamic room list management |
| Object add/remove | `editor.c` | Dynamic object list management |
| Room picker UI | `editor.c` | Select which room to place on void cell |

## Files to Modify

| File | Changes |
|------|---------|
| `editor.c` | All new editor features (room numbers, borders, room placement, add/delete rooms, object editing) |
| `engine.c` | Add `setscreen()` function |
| `engine.h` | Declare `setscreen()` |
| `world.c` | Ensure `save_world()` handles dynamic arrays correctly |
| `world.h` | No changes needed |

## Implementation Order

1. **Setscreen function** (prerequisite for room placement)
2. **Map room number display** (visual enhancement)
3. **Map border rendering** (visual enhancement)
4. **Room picker + placement** (void cell filling)
5. **Add/delete rooms** (room list management)
6. **Object placement mode** (object editing)
