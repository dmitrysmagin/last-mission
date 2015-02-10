/*
*/

typedef struct {
	int x, y;
	int index;
} PATTERN;

typedef struct {
	int x, y;
	int index;
	int speed;
	int minframe, maxframe;
	int ai;
	int garage_id;
} OBJECT;

typedef struct {
	int x1, y1, x2, y2;
} BGLINE;

typedef struct {
	int xs, ys;
	char *data;
} BGMAP;

typedef struct {
	int xs, ys; // room dimensions in 8x8 tiles

	int pattern_num;	// number of PATTERN * entries
	int object_num;		// number of OBJECT * entries
	int bg_type;		// lines or bgspritemap
	int bg_num;		// number of background entries

	// color values for background
	unsigned int background;
	unsigned int shadow;
	unsigned int line_light;
	unsigned int line_shadow;

	int up, right, down, left; // screens
	int procedure; // 3 - win screen
	int bonus;

	PATTERN *pattern;
	OBJECT *object;

	union {
		BGLINE *bgline;
		BGMAP *bgmap;
	};
} ROOM;

typedef struct {
	int xs, ys;
	char *data;
} PATTERNSET;

typedef struct {
	char xs, ys;
	char data[];
} SPRITE;

typedef struct {
	int num; // number of sprites
	SPRITE *sprite;
} SPRITESET;

typedef struct {
	int room_num;
	int patternset_num;
	int spriteset_num;
	int tileset_num;
	int fontset_num;
	int bgspriteset_num;

	ROOM *room;
	PATTERNSET *patternset;
	SPRITESET *spriteset;
	char *tileset;
	SPRITESET *bgspriteset;
	char *fontset;
} WORLD;

// ============================================================================

WORLD *load_world(char *name);
void save_world(char *name, WORLD *world);
