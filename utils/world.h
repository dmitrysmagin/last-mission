/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS for(int A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _WORLD_H_
#define _WORLD_H_

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
		BGMAP *bgmap;	// not used yet
	};
} ROOM;

typedef struct {
	int xs, ys;
	char *data;
} PATTERNSET;

typedef struct {
	char xs, ys;
	char data[];
} SPRITE;			// not used yet

typedef struct {
	int num; // number of sprites
	SPRITE *sprite;
} SPRITESET;			// not used yet

typedef struct {
	int room_num;
	int patternset_num;
	int spriteset_num;
	int tileset_num;	// not used yet
	int fontset_num;	// not used yet
	int bgspriteset_num;	// not used yet

	ROOM *room;
	PATTERNSET *patternset;
	SPRITESET *spriteset;	// not used yet
	char *tileset;		// not used yet
	SPRITESET *bgspriteset;	// not used yet
	char *fontset;		// not used yet
} WORLD;

// ============================================================================

WORLD *load_world(char *name);
void save_world(char *name, WORLD *world);
void free_world(WORLD *world);

#endif /* _WORLD_H_ */
