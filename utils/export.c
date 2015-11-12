/*
 *
 *  exp.c - a tool to export inner data to world format
 *
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m_scr.c"
#include "screenlines/m_scr_lines.c"
#include "m_data.c"
#include "../world.c"

WORLD *world;

int main()
{
	int i, j;

	// allocate WORLD structure
	world = (WORLD *)calloc(1, sizeof(WORLD));

	// allocate an array of ROOM's
	world->room_num = NUM_SCREENS;
	world->room = (ROOM *)calloc(NUM_SCREENS, sizeof(ROOM));

	// fill room data
	for(i = 0; i < NUM_SCREENS; i++) {
		ROOM *room = world->room + i;
		unsigned char *p;

		// fill info on patterns
		p = SCREENS[i];

		room->pattern_num = p[0];
		room->pattern = (PATTERN *)calloc(p[0], sizeof(PATTERN));

		for(j = 0, p += 1; j < room->pattern_num; j++, p += 4) {
			PATTERN *pattern = room->pattern + j;

			pattern->x = p[0];
			pattern->y = p[1];
			pattern->index = p[2];
		}

		// fill info on moving objects (enemies)
		p = SCREENINFOS[i];

		room->up    = p[0];
		room->right = p[1];
		room->down  = p[2];
		room->left  = p[3];

		room->object_num = p[4];
		room->object     = (OBJECT *)calloc(p[4], sizeof(OBJECT));

		for(j = 0, p += 5; j < room->object_num; j++, p += 8) {
			OBJECT *object = room->object + j;

			object->garage_id = p[0];
			object->index     = p[1];
			object->x         = p[2] * 4;
			object->y         = p[3];
			object->speed     = p[4] * 2;
			object->minframe  = p[5];
			object->maxframe  = p[6];
			object->ai        = p[7];
		}

		room->procedure = p[0];
		room->bonus     = p[1];

		// fill info on background
		if (i <= 69 || i >= 92) { // if not on planet surface
			short *p = SCREENLINES[i];

			room->bg_type     = 1; // generated background
			room->bg_num      = p[0]; // number of BGLINE * entries
			room->background  = GetScreenDrawInfo(i)->background;
			room->shadow      = GetScreenDrawInfo(i)->shadow;
			room->line_light  = GetScreenDrawInfo(i)->line_light;
			room->line_shadow = GetScreenDrawInfo(i)->line_shadow;

			room->bgline = (BGLINE *)calloc(p[0], sizeof(BGLINE));

			for (j = 0, p++; j < room->bg_num; j++, p += 4) {
				BGLINE *bgline = room->bgline + j;

				bgline->x1 = p[0];
				bgline->y1 = p[1];
				bgline->x2 = p[2];
				bgline->y2 = p[3];
			}
		} else {
			room->bg_type = 0;
			room->bg_num  = 0;
		}
	}

	// allocate an array of PATTERNSET's
	world->patternset_num = NUM_PATTERNS;
	world->patternset = (PATTERNSET *)calloc(NUM_PATTERNS, sizeof(PATTERNSET));

	// fill PATTERNSET data
	for(i = 0; i < NUM_PATTERNS; i++) {
		PATTERNSET *patternset = world->patternset + i;
		unsigned char *p;
		int size;

		p = PATTERNS[i];
		patternset->xs = p[0];
		patternset->ys = p[1];
		size = p[0] * p[1];

		patternset->data = malloc(size);
		memcpy(patternset->data, p+2, size);
	}

	// save data to file
	save_world("lastmission.dat", world);

	// deallocate everything
	for(i = 0; i < NUM_PATTERNS; i++) {
		free((world->patternset + i)->data);
	}
	free(world->patternset);

	for(i = 0; i < NUM_SCREENS; i++) {
		free((world->room + i)->object);
		free((world->room + i)->pattern);
	}
	free(world->room);
	free(world);

	// load again for testing :)
	world = load_world("lastmission.dat");

	return 0;
}


















