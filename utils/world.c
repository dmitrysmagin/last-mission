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

/*
 * Routines for reading/writing world .dat files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world.h"

//#define DEBUG

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF(...)
#endif

static void fwrite_HEADER(FILE *fp)
{
	fputs("# The Last Mission data file, do not edit!\n",fp);
	fputs("\n",fp);

	fputs("LASTMISSION 1\n", fp);
}

// don't use sizeof(WORLD) because it saves pointers, trim them instead
#define SIZEOF_WORLD (sizeof(int) * 6)

static void fwrite_WORLD(WORLD *world, FILE *fp)
{
	char g_string[256];

	sprintf(g_string, "WORLD %d %d %d %d %d %d\n",
		world->room_num,
		world->patternset_num,
		world->spriteset_num,
		world->tileset_num,
		world->fontset_num,
		world->bgspriteset_num);

	fputs(g_string, fp);
}

static void fread_WORLD(WORLD *world, FILE *fp)
{
}

// don't use sizeof(ROOM) because it saves pointers, trim them instead
#define SIZEOF_ROOM (sizeof(ROOM) - sizeof(int*) * 3)

static void fwrite_ROOM(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "ROOM %d %d %d %d %d %d %x %x %x %x %d %d %d %d %d %d\n",
			room->xs, room->ys,
			room->pattern_num,
			room->object_num,
			room->bg_type,
			room->bg_num,
			room->background, room->shadow, room->line_light, room->line_shadow,
			room->up, room->right, room->down, room->left,
			room->procedure,
			room->bonus);
		fputs(g_string, fp);
	}
}

static void fread_ROOM(WORLD *world, FILE *fp)
{
}

static void fwrite_ROOM_PATTERN(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "PATTERN # for ROOM %d\n", i);
		fputs(g_string, fp);

		for (int j = 0; j < room->pattern_num; j++) {
			sprintf(g_string, "\t%d %d %d\n",
				(room->pattern + j)->x,
				(room->pattern + j)->y,
				(room->pattern + j)->index);
			fputs(g_string, fp);
		}
	}
}

static void fread_ROOM_PATTERN(WORLD *world, FILE *fp)
{
}

static void fwrite_ROOM_OBJECT(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "OBJECT # for ROOM %d\n", i);
		fputs(g_string, fp);

		for (int j = 0; j < room->object_num; j++) {
			sprintf(g_string, "\t%d %d %d %d %d %d %d %d\n",
				(room->object + j)->x,
				(room->object + j)->y,
				(room->object + j)->index,
				(room->object + j)->speed,
				(room->object + j)->minframe,
				(room->object + j)->maxframe,
				(room->object + j)->ai,
				(room->object + j)->garage_id);
			fputs(g_string, fp);
		}
	}
}

static void fread_ROOM_OBJECT(WORLD *world, FILE *fp)
{
}

static void fwrite_ROOM_BGLINE(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "BGLINE # for ROOM %d\n", i);
		fputs(g_string, fp);

		for (int j = 0; j < room->bg_num; j++) {
			sprintf(g_string, "\t%d %d %d %d\n",
				(room->bgline + j)->x1,
				(room->bgline + j)->y1,
				(room->bgline + j)->x2,
				(room->bgline + j)->y2);
			fputs(g_string, fp);
		}
	}
}

static void fread_ROOM_BGLINE(WORLD *world, FILE *fp)
{
}

static void fwrite_PATTERNSET_array(int pxs, int ys, char *data, FILE *fp)
{
	char g_string[256];

	for (; ys > 0; ys--) {
		strcpy(g_string, "\t");
		for (int xs = pxs; xs > 0; xs--) {
			sprintf(g_string + strlen(g_string), " %hhu", *data++);
		}
		strcat(g_string, "\n");
		fputs(g_string, fp);
	}
}

static void fwrite_PATTERNSET(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->patternset_num; i++) {
		PATTERNSET *patternset = world->patternset + i;

		sprintf(g_string, "PATTERNSET %d %d\n",
			patternset->xs, patternset->ys);
		fputs(g_string, fp);

		fwrite_PATTERNSET_array(patternset->xs,
					patternset->ys,
					patternset->data, fp);
	}
}

static void fread_PATTERNSET(WORLD *world, FILE *fp)
{
}

void save_world(char *name, WORLD *world)
{
	FILE *fp;

	if (!world)
		return;

	fp = fopen(name, "w");

	fwrite_HEADER(fp);
	fwrite_WORLD(world, fp);
	fwrite_ROOM(world, fp);
	fwrite_ROOM_PATTERN(world, fp);
	fwrite_ROOM_OBJECT(world, fp);
	fwrite_ROOM_BGLINE(world, fp);
	fwrite_PATTERNSET(world, fp);

	fclose(fp);
}


/*
 *  Load data from .dat file allocating all pointers
 */

WORLD *load_world(char *name)
{
	WORLD *world;
	FILE *fp;

	world = (WORLD *)calloc(1, sizeof(WORLD));

	fp = fopen(name, "r");

	//fread_HEADER(fp);
	fread_WORLD(world, fp);
	fread_ROOM(world, fp);
	fread_ROOM_PATTERN(world, fp);
	fread_ROOM_OBJECT(world, fp);
	fread_ROOM_BGLINE(world, fp);
	fread_PATTERNSET(world, fp);

	fclose(fp);
	return world;
}

void free_world(WORLD *world)
{
	int i;

	// roll through all rooms
	for (i = 0; i < world->room_num; i++) {
		if ((world->room + i)->pattern) {
			free((world->room + i)->pattern);
			(world->room + i)->pattern = NULL;
		}

		if ((world->room + i)->object) {
			free((world->room + i)->object);
			(world->room + i)->object = NULL;
		}

		if ((world->room + i)->bgline) {
			free((world->room + i)->bgline);
			(world->room + i)->bgline = NULL;
		}
	}

	if (world->room) {
		free(world->room);
		world->room = NULL;
	}

	if (world->patternset) {
		free(world->patternset);
		world->patternset = NULL;
	}

	free(world);
}
