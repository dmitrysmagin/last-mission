# ENGINE_REFACTOR.md - File Naming Convention

All new engine-related files use **dash naming convention** (not underscore):

## Core Engine Files
- `engine-interface.h` - Strategy interface definition
- `engine-flipscreen.c` - Flip-screen engine implementation
- `engine-scroller.c` - Scrolling engine implementation

## Helper Module Files  
- `engine-coordinates.h` / `engine-coordinates.c` - Coordinate system abstraction
- `engine-collision.h` / `engine-collision.c` - Collision system abstraction  
- `engine-object.h` - Object pooling extensions

## Existing Files (unchanged)
- `engine.c` - Main game logic (to be refactored)
- `engine-scrolling.c` - Scrolling implementation (to be replaced)
- `engine.h` - Engine headers
- `room.c` / `room.h` - Room management

**Note:** Only the *new* files created during refactoring use dashes. Existing source files keep their original naming.

---

# Architectural Analysis: Flip-Screen vs Scrolling Engines

## 1. Current State

### Code Characteristics
- **24 GM_SCROLLING checks** scattered throughout the codebase
- **Conditional sprawl** in camera updates, object rendering, and collision detection
- **Tight coupling** between game logic and rendering implementation
- **Context switching overhead** when toggling between modes

### Problem Areas
- `engine.c:processGameLoop()` - Mixed mode detection and handling
- `camera.c:updateCamera()` -conditional viewport calculations
- `collision.c:checkObjectCollisions()` - Mode-specific collision algorithms
- `render.c:renderScene()` - Separate rendering paths for each mode

### Technical Debt
- Duplicate object update logic
- Inconsistent coordinate handling
- Difficulty adding new engine variants
- Testing complexity due to branching logic

## 2. Proposed Architecture

### Strategy Pattern Implementation
```
┌─────────────────────────────────────────────┐
│           Game Loop (engine.c)              │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│       IGameEngine (interface)               │
│  - update()                                 │
│  - render()                                 │
│  - handleInput()                            │
│  - getCameraX() / getCameraY()              │
└─────────────────┬───────────────────────────┘
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
┌─────────────────┐ ┌─────────────────┐
│ FlipScreenEngine│ │ ScrollerEngine  │
│ (current mode)  │ │ (target mode)   │
└─────────────────┘ └─────────────────┘
```

### Benefits
- Single responsibility principle
- Open/closed principle (extensible)
- Eliminates conditional complexity
- Easier testing (mockable interfaces)

## 3. Core Interface Definition (engine-interface.h)

```c
#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H

#include "types.h"
#include "object.h"

typedef struct IGameEngine IGameEngine;

typedef void (*EngineUpdateFunc)(IGameEngine* self, Gamestate* gs);
typedef void (*EngineRenderFunc)(IGameEngine* self, Gamestate* gs);
typedef void (*EngineHandleInputFunc)(IGameEngine* self, Gamestate* gs, InputState* input);
typedef void (*EngineCleanupFunc)(IGameEngine* self);

typedef struct {
    int x;
    int y;
} CameraPosition;

typedef struct IGameEngineVtbl {
    EngineUpdateFunc update;
    EngineRenderFunc render;
    EngineHandleInputFunc handleInput;
    EngineCleanupFunc cleanup;
} IGameEngineVtbl;

struct IGameEngine {
    const IGameEngineVtbl* vtable;
    CameraPosition camera;
    void* engineData;
};

IGameEngine* createFlipScreenEngine(void);
IGameEngine* createScrollerEngine(void);
void destroyEngine(IGameEngine* engine);

#endif
```

## 4. Engine Implementations

### engine-flipscreen.c
```c
#include "engine-interface.h"
#include "camera.h"
#include "render.h"

static void flipScreenUpdate(IGameEngine* self, Gamestate* gs) {
    // Current flip-screen logic preserved
    updateFlipScreenCamera(&self->camera, gs);
    updateObjectPositions(gs);
    checkCollisions(gs);
}

static void flipScreenRender(IGameEngine* self, Gamestate* gs) {
    renderFlipScreen(gs, &self->camera);
}

static const IGameEngineVtbl flipScreenVtbl = {
    .update = flipScreenUpdate,
    .render = flipScreenRender,
    .handleInput = NULL,  // Shared implementation
    .cleanup = NULL
};

IGameEngine* createFlipScreenEngine(void) {
    IGameEngine* engine = malloc(sizeof(IGameEngine));
    engine->vtable = &flipScreenVtbl;
    engine->camera.x = 0;
    engine->camera.y = 0;
    return engine;
}
```

### engine-scroller.c
```c
#include "engine-interface.h"
#include "engine-coordinates.h"
#include "render.h"

static void scrollerUpdate(IGameEngine* self, Gamestate* gs) {
    updateScrollerCamera(&self->camera, gs);
    updateObjectPositions(gs);
    checkCollisions(gs);
}

static void scrollerRender(IGameEngine* self, Gamestate* gs) {
    renderScroller(gs, &self->camera);
}

static const IGameEngineVtbl scrollerVtbl = {
    .update = scrollerUpdate,
    .render = scrollerRender,
    .handleInput = NULL,
    .cleanup = NULL
};

IGameEngine* createScrollerEngine(void) {
    IGameEngine* engine = malloc(sizeof(IGameEngine));
    engine->vtable = &scrollerVtbl;
    engine->camera.x = 0;
    engine->camera.y = 0;
    return engine;
}
```

## 5. Game State Refactoring

### Current State Issues
```c
// Old approach - mixed concerns
typedef struct {
    int mode;           // GM_SCROLLING flag
    int camera_x;
    int camera_y;
    // ... other fields
} Gamestate;
```

### New Approach
```c
typedef struct {
    IGameEngine* engine;  // Strategy object
    // Mode-specific data delegated to engine
    // Generic game data remains here
} Gamestate;

// Access via interface
int cameraX = gamestate->engine->camera.x;
gamestate->engine->vtable->update(gamestate->engine, gamestate);
```

## 6. Object Layer Integration

### engine-object.h Extensions
```c
// New functions for engine-agnostic object handling
void object_updateForEngine(Object* obj, IGameEngine* engine, Gamestate* gs);
void object_renderForEngine(Object* obj, IGameEngine* engine, Gamestate* gs);
int object_getScreenX(Object* obj, IGameEngine* engine);
int object_getScreenY(Object* obj, IGameEngine* engine);
```

### Implementation Pattern
```c
// engine-coordinates.c
int object_getScreenX(Object* obj, IGameEngine* engine) {
    if (obj->flags & OBJECT_WORLD_COORDS) {
        return worldToScreenX(obj->x, engine);
    }
    return obj->x;  // Already screen coordinates
}
```

## 7. Game Loop Integration

### engine.c Refactoring
```c
void processGameLoop(Gamestate* gs) {
    // Input handling
    handleInput(gs);
    
    // Engine agnostic state updates
    updatePlayer(gs);
    updateEnemies(gs);
    
    // Delegated to strategy
    gs->engine->vtable->update(gs->engine, gs);
    gs->engine->vtable->render(gs->engine, gs);
}
```

### Runtime Engine Switching
```c
void setEngineMode(Gamestate* gs, EngineMode mode) {
    if (gs->engineMode == mode) return;
    
    IGameEngine* oldEngine = gs->engine;
    
    switch (mode) {
        case ENGINE_FLIPSCREEN:
            gs->engine = createFlipScreenEngine();
            break;
        case ENGINE_SCROLLER:
            gs->engine = createScrollerEngine();
            break;
    }
    
    gs->engineMode = mode;
    destroyEngine(oldEngine);
}
```

## 8. Benefits Table

| Aspect | Current (Conditional) | Proposed (Strategy) |
|--------|----------------------|---------------------|
| **Lines of Code** | ~1200 (with duplication) | ~600 (separated) |
| **GM_SCROLLING Checks** | 24 scattered | 0 in logic code |
| **New Engine Addition** | Modify all conditionals | Single new class |
| **Test Complexity** | High (branch coverage) | Low (isolated tests) |
| **Camera Code** | 400+ lines | ~150 per engine |
| **Object Updates** | Duplicated logic | Single generic |

## 9. Technical Considerations

### Memory Overhead
- Strategy object: ~32 bytes per game state
- Engine data pointers: minimal
- Tradeoff: memory for maintainability

### Performance
- Virtual function call overhead: negligible (~4 bytes pointer dereference)
- Memory locality: slightly reduced (indirect calls)
- Cache behavior: similar to current implementation

### Compatibility
- Maintains backward compatibility with existing assets
- No changes to level file format required
- Engine mode is runtime switchable

### Data Migration
```c
// When switching engines, preserve camera position
void migrateCamera(IGameEngine* oldEngine, IGameEngine* newEngine) {
    newEngine->camera = oldEngine->camera;
}
```

## 10. Migration Path

### Phase 1: Interface Definition
- [ ] Create `engine-interface.h`
- [ ] Implement `engine-flipscreen.c` (preserves current behavior)
- [ ] Implement skeleton `engine-scroller.c`

### Phase 2: Game State Integration
- [ ] Add `IGameEngine* engine` field to `Gamestate`
- [ ] Implement `setEngineMode()` function
- [ ] Refactor engine creation at game startup

### Phase 3: Object Layer
- [ ] Create `engine-coordinates.h`/`.c`
- [ ] Refactor object rendering to use engine
- [ ] Convert all `getScreenX/Y` calls

### Phase 4: Collision System
- [ ] Create `engine-collision.h`/`.c`
- [ ] Extract collision logic per engine
- [ ] Test both modes

### Phase 5: Cleanup
- [ ] Remove `GM_SCROLLING` checks
- [ ] Remove old `engine-scrolling.c`
- [ ] Update documentation

## 11. Example Object Update

### Before
```c
void updateEnemy(Enemy* enemy, Gamestate* gs) {
    if (gs->mode & GM_SCROLLING) {
        // Scroller logic with world coordinates
        enemy->x += enemy->dx;
        enemy->y += enemy->dy;
        if (enemy->x < 0 || enemy->x > MAP_WIDTH * 16) {
            enemy->dx = -enemy->dx;
        }
    } else {
        // Flip-screen logic with screen coordinates
        enemy->x += enemy->dx;
        if (enemy->x < 0 || enemy->x > SCREEN_WIDTH) {
            enemy->dx = -enemy->dx;
        }
    }
}
```

### After
```c
void updateEnemy(Enemy* enemy, Gamestate* gs) {
    // Generic movement
    enemy->x += enemy->dx;
    enemy->y += enemy->dy;
    
    // Engine delegates boundary checking
    if (object_isOutOfBounds(enemy, gs->engine, gs)) {
        enemy->dx = -enemy->dx;
    }
}

// engine-coordinates.c
int object_isOutOfBounds(Object* obj, IGameEngine* engine, Gamestate* gs) {
    int screenX = object_getScreenX(obj, engine);
    int screenY = object_getScreenY(obj, engine);
    
    if (engine->mode == ENGINE_SCROLLER) {
        return (screenX < 0 || screenX > MAP_WIDTH * 16 ||
                screenY < 0 || screenY > MAP_HEIGHT * 16);
    } else {
        return (screenX < 0 || screenX > SCREEN_WIDTH ||
                screenY < 0 || screenY > SCREEN_HEIGHT);
    }
}
```

## 12. Summary

The Strategy Pattern refactoring eliminates the conditional complexity of the current implementation by:

- **Seperating concerns**: Each engine mode has dedicated code
- **Improving extensibility**: Adding new modes requires minimal changes
- **Reducing bugs**: Eliminates incorrect branching in game logic
- **Simplifying tests**: Each engine can be tested independently

The migration can be performed incrementally without disrupting ongoing development, as both old and new patterns can coexist during the transition period.

---

## 11. Helper Functions: Coordinate Conversion

### engine-coordinates.h
```c
#ifndef ENGINE_COORDINATES_H
#define ENGINE_COORDINATES_H

#include "engine-interface.h"
#include "types.h"

// World to Screen conversion
int worldToScreenX(int worldX, IGameEngine* engine);
int worldToScreenY(int worldY, IGameEngine* engine);

// Screen to World conversion  
int screenToWorldX(int screenX, IGameEngine* engine);
int screenToWorldY(int screenY, IGameEngine* engine);

// Object coordinate helpers
int object_getScreenX(Object* obj, IGameEngine* engine);
int object_getScreenY(Object* obj, IGameEngine* engine);
void object_setWorldPos(Object* obj, int worldX, int worldY, IGameEngine* engine);

// Room boundary helpers
int getRoomColumns(IGameEngine* engine);
int getRoomRows(IGameEngine* engine);

#endif
```

### engine-coordinates.c (Key Functions)
```c
int worldToScreenX(int worldX, IGameEngine* engine) {
    // Scroller: subtract camera offset
    // FlipScreen: modulo room width
    return worldX - engine->camera.x;
}

int worldToScreenY(int worldY, IGameEngine* engine) {
    return worldY - engine->camera.y;
}

int getRoomColumns(IGameEngine* engine) {
    // Scroller: full map width
    // FlipScreen: fixed room width (16 tiles)
    return (engine->mode == ENGINE_SCROLLER) ? 
           (MAP_WIDTH / 16) : 16;
}
```

## 12. Room-vs-Map Coordinates

### Coordinate Systems

**World Coordinates**
- Continuous space across entire map
- Used internally for object positions
- Range: 0 to MAP_WIDTH * 16, 0 to MAP_HEIGHT * 16

**Screen Coordinates**
- Relative to camera viewport
- Used for rendering
- Range: 0 to SCREEN_WIDTH, 0 to SCREEN_HEIGHT

**Room Coordinates** (Flip-screen only)
- 16x16 tile rooms
- Discrete room indices
- Range: 0-15 rooms in each dimension

### Conversion Matrix

| From\To | World | Screen | Room |
|---------|-------|--------|------|
| World   | - | `worldToScreen()` | `worldToRoom()` |
| Screen  | `screenToWorld()` | - | `screenToRoom()` |
| Room    | `roomToWorld()` | `roomToScreen()` | - |

## 13. Collision Detection

### engine-collision.h
```c
#ifndef ENGINE_COLLISION_H
#define ENGINE_COLLISION_H

#include "engine-interface.h"
#include "object.h"

// Main collision dispatcher
void checkCollisions(Gamestate* gs);

// Per-engine implementations
void checkCollisionsFlipScreen(Gamestate* gs);
void checkCollisionsScroller(Gamestate* gs);

// Collision helpers
int objects_overlap(Object* a, Object* b, IGameEngine* engine);
int object_inBounds(Object* obj, IGameEngine* engine);
int object_inRoom(Object* obj, int roomX, int roomY, IGameEngine* engine);

#endif
```

### Implementation Strategy

**FlipScreen Collision**
- Check against fixed 16x16 room boundaries
- Simplified collision grid (256 rooms)
- No partial room overlaps

**Scroller Collision**
- Continuous world boundaries
- Check against map edges
- Partial room/viewport overlaps handled

```c
void checkCollisions(Gamestate* gs) {
    gs->engine->vtable->checkCollisions(gs);
}
```

## 14. Enemy Objects Handling

### Engine-Specific Behavior

**Flip-Screen Enemies**
- Confined to single room
- Respawn logic per room
- Simple patrol patterns (room-local)

**Scroller Enemies**
- Can traverse multiple rooms
- Persistent state across rooms
- Complex movement patterns

### Object flags Extension
```c
// engine-object.h
#define OBJECT_WORLD_COORDS     0x10  // Uses world coordinates
#define OBJECT_ROOM_COORDS      0x20  // Uses room coordinates  
#define OBJECT_ENEMY            0x40
#define OBJECT_ENEMY_SCROLLER   0x80  // Scroller-specific enemy
```

### Enemy Update Pattern
```c
void updateEnemy(Enemy* enemy, Gamestate* gs) {
    if (enemy->flags & OBJECT_ENEMY_SCROLLER) {
        updateScrollerEnemy(enemy, gs);
    } else {
        updateFlipScreenEnemy(enemy, gs);
    }
}

// Generic dispatcher via engine
void enemy_update(Enemy* enemy, Gamestate* gs) {
    if (gs->engine->mode == ENGINE_SCROLLER) {
        updateScrollerEnemy(enemy, gs);
    } else {
        updateFlipScreenEnemy(enemy, gs);
    }
}
```

## 15. AI Processing

### Current State
```c
// engine.c - Mixed AI logic
void processAI(Gamestate* gs) {
    for each enemy {
        if (gs->mode & GM_SCROLLING) {
            // Scroller AI (line-of-sight, tracking)
            updateScrollerAI(enemy, gs);
        } else {
            // Flip-screen AI (patrol, simple tracking)
            updateFlipScreenAI(enemy, gs);
        }
    }
}
```

### Refactored AI System
```c
// ai-engine.h
typedef struct EnemyAI {
    void (*update)(Enemy*, Gamestate*, IGameEngine* engine);
    void (*init)(Enemy*, Gamestate*, IGameEngine* engine);
} EnemyAI;

// Per-engine AI handlers
EnemyAI scrollerAI = {
    .update = updateScrollerAI,
    .init = initScrollerAI
};

EnemyAI flipScreenAI = {
    .update = updateFlipScreenAI,
    .init = initFlipScreenAI
};

// In game state
typedef struct {
    // ... existing fields
    EnemyAI* currentAI;
} Gamestate;
```

### AI Initialization Pattern
```c
void initEnemyAI(Enemy* enemy, Gamestate* gs) {
    if (gs->engine->mode == ENGINE_SCROLLER) {
        scrollerAI.init(enemy, gs, gs->engine);
        enemy->ai = &scrollerAI;
    } else {
        flipScreenAI.init(enemy, gs, gs->engine);
        enemy->ai = &flipScreenAI;
    }
}
```

### Key AI Differences

| Feature | Flip-Screen | Scroller |
|---------|-------------|----------|
| Detection Range | Per-room | Line-of-sight |
| Tracking | Simple | Predictive |
| Pathfinding | Room grid | Continuous |
| State Persistence | Room-local | Global |

---

*Document version: 1.0*
*Last updated: Engine refactoring analysis complete*
