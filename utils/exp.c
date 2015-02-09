/*
 *
 *  exp.c - a tool to export inner data to world format
 *
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../m_data.c"
#include "../m_scr.c"
#include "../m_scr_lines.c"
#include "../world.c"

WORLD *world;

int main()
{
	int i, j;

	// allocate WORLD structure
	world = (WORLD *)malloc(sizeof(WORLD));
	memset(world, 0, sizeof(WORLD));

	// allocate an array of ROOM's
	world->room_num = NUM_SCREENS;
	world->room = (ROOM *)malloc(sizeof(ROOM) * NUM_SCREENS);
	memset(world->room, 0, sizeof(ROOM) * NUM_SCREENS);

	// fill room data
	for(i = 0; i < NUM_SCREENS; i++) {
		char *p;

		// fill info on patterns
		p = SCREENS[i];

		(world->room + i)->pattern_num = j = *(p+0);
		(world->room + i)->pattern = (PATTERN *)malloc(sizeof(PATTERN) * j);

		for(j = 0, p += 1; j < (world->room + i)->pattern_num; j++, p += 4) {
			((world->room + i)->pattern + j)->x = *(p+0);
			((world->room + i)->pattern + j)->y = *(p+1);
			((world->room + i)->pattern + j)->index = *(p+2);
		}

		// fill info on moving objects (enemies)
		p = SCREENINFOS[i];

		(world->room + i)->up = *p;
		(world->room + i)->right = *(p+1);
		(world->room + i)->down = *(p+2);
		(world->room + i)->left = *(p+3);

		(world->room + i)->object_num = j = *(p+4);
		(world->room + i)->object = (OBJECT *)malloc(sizeof(OBJECT) * j);

		for(j = 0, p += 5; j < (world->room + i)->object_num; j++, p += 8) {
			((world->room + i)->object + j)->garage_id = *(p+0);
			((world->room + i)->object + j)->index = *(p+1);
			((world->room + i)->object + j)->x = *(p+2);
			((world->room + i)->object + j)->y = *(p+3);

			((world->room + i)->object + j)->speed = *(p+4);
			((world->room + i)->object + j)->minframe = *(p+5);
			((world->room + i)->object + j)->maxframe = *(p+6);
			((world->room + i)->object + j)->ai = *(p+7);
		}

		(world->room + i)->procedure = *(p+0);
		(world->room + i)->bonus = *(p+1);
	}

	// allocate an array of PATTERNSET's
	world->patternset_num = NUM_PATTERNS;
	world->patternset = (PATTERNSET *)malloc(sizeof(PATTERNSET) * NUM_PATTERNS);
	memset(world->patternset, 0, sizeof(PATTERNSET) * NUM_PATTERNS);

	// fill PATTERNSET data
	for(i = 0; i < NUM_PATTERNS; i++) {
		char *p;
		int size;

		p = PATTERNS[i];
		(world->patternset + i)->xs = *(p+0);
		(world->patternset + i)->ys = *(p+1);
		size = *(p+0) * *(p+1);

		(world->patternset + i)->data = malloc(size);
		memcpy((world->patternset + i)->data, p+2, size);
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


















