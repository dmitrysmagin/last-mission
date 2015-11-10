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

static int fread_HEADER(FILE *fp)
{
	char g_string[256];
	char arg1[128];
	int  arg2;

	while (fgets(g_string, sizeof(g_string), fp)) {
		sscanf(g_string, "%s %d", &arg1, &arg2);

		if (strcmp(arg1, "#") != 0) {
			if (arg2 == 1) {
				DPRINTF("LASTMISSION header version %d found\n", arg2);

				return 1;
			}
		}
	}

	DPRINTF("Error reading LASTMISSION header\n");

	return 0;
}

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

static int fread_WORLD(WORLD *world, FILE *fp)
{
	char g_string[256];
	char arg1[128];

	while (fgets(g_string, sizeof(g_string), fp)) {
		sscanf(g_string, "%s %d %d %d %d %d %d",
			&arg1,
			&world->room_num,
			&world->patternset_num,
			&world->spriteset_num,
			&world->tileset_num,
			&world->fontset_num,
			&world->bgspriteset_num);

		if ((strcmp(arg1, "#") != 0) && (strcmp(arg1, "WORLD") == 0)) {
			DPRINTF("WORLD chunk found; ROOMs: %d, PATTERNSETs: %d\n",
				world->room_num, world->patternset_num);

			/* allocate all necessary pointers */
			world->room = (ROOM *)
				calloc(world->room_num, sizeof(ROOM));
			world->patternset = (PATTERNSET *)
				calloc(world->patternset_num, sizeof(PATTERNSET));

			return 1;
		}
	}

	DPRINTF("Error reading WORLD chunk\n");

	return 0;
}

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

static int fread_ROOM(WORLD *world, FILE *fp)
{
	char g_string[256];
	char arg1[128];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		fgets(g_string, sizeof(g_string), fp);
		sscanf(g_string, "%s %d %d %d %d %d %d %x %x %x %x %d %d %d %d %d %d",
			&arg1,
			&room->xs, &room->ys,
			&room->pattern_num,
			&room->object_num,
			&room->bg_type,
			&room->bg_num,
			&room->background, &room->shadow, &room->line_light, &room->line_shadow,
			&room->up, &room->right, &room->down, &room->left,
			&room->procedure,
			&room->bonus);

		if (strcmp(arg1, "ROOM") == 0) {
			/* allocate all necessary pointers */
			room->pattern = (PATTERN *)
				calloc(room->pattern_num, sizeof(PATTERN));
			room->object = (OBJECT *)
				calloc(room->object_num, sizeof(OBJECT));

			room->bgline = (BGLINE *)
				calloc(room->bg_num, sizeof(BGLINE));

			DPRINTF("ROOM chunk %d found; PATTERNs: %d; OBJECTs: %d; BGLINEs: %d\n",
				i, room->pattern_num, room->object_num, room->bg_num);
		} else {
			DPRINTF("Error reading ROOM chunk %d\n", i);

			return 0;
		}
	}

	return 1;
}

static void fwrite_ROOM_PATTERN(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "PATTERN %d\n", i);
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

static int fread_ROOM_PATTERN(WORLD *world, FILE *fp)
{
	char g_string[256];
	char arg1[128];
	int  arg2;

	for (int i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		fgets(g_string, sizeof(g_string), fp);
		sscanf(g_string, "%s %d", &arg1, &arg2);

		if (strcmp(arg1, "PATTERN") != 0 && arg2 != i) {
			DPRINTF("Error reading PATTERN chunk %d\n", i);

			return 0;
		}

		for (int j = 0; j < room->pattern_num; j++) {
			fgets(g_string, sizeof(g_string), fp);
			sscanf(g_string, "%d %d %d",
				&(room->pattern + j)->x,
				&(room->pattern + j)->y,
				&(room->pattern + j)->index);
		}
	}

	return 1;
}

static void fwrite_ROOM_OBJECT(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "OBJECT %d\n", i);
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

static int fread_ROOM_OBJECT(WORLD *world, FILE *fp)
{
	char g_string[256];
	char arg1[128];
	int  arg2;

	for (int i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		fgets(g_string, sizeof(g_string), fp);
		sscanf(g_string, "%s %d", &arg1, &arg2);

		if (strcmp(arg1, "OBJECT") != 0 && arg2 != i) {
			DPRINTF("Error reading OBJECT chunk %d\n", i);

			return 0;
		}

		for (int j = 0; j < room->object_num; j++) {
			fgets(g_string, sizeof(g_string), fp);
			sscanf(g_string, "%d %d %d %d %d %d %d %d",
				&(room->object + j)->x,
				&(room->object + j)->y,
				&(room->object + j)->index,
				&(room->object + j)->speed,
				&(room->object + j)->minframe,
				&(room->object + j)->maxframe,
				&(room->object + j)->ai,
				&(room->object + j)->garage_id);
		}
	}

	return 1;
}

static void fwrite_ROOM_BGLINE(WORLD *world, FILE *fp)
{
	char g_string[256];
	int i;

	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		sprintf(g_string, "BGLINE %d\n", i);
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

static int fread_ROOM_BGLINE(WORLD *world, FILE *fp)
{
	char g_string[256];
	char arg1[128];
	int  arg2;

	for (int i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		fgets(g_string, sizeof(g_string), fp);
		sscanf(g_string, "%s %d", &arg1, &arg2);

		if (strcmp(arg1, "BGLINE") != 0 && arg2 != i) {
			DPRINTF("Error reading BGLINE chunk %d\n", i);

			return 0;
		}

		for (int j = 0; j < room->bg_num; j++) {
			fgets(g_string, sizeof(g_string), fp);
			sscanf(g_string, "%d %d %d %d",
				&(room->bgline + j)->x1,
				&(room->bgline + j)->y1,
				&(room->bgline + j)->x2,
				&(room->bgline + j)->y2);
		}
	}

	return 1;
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

static void fread_PATTERNSET_array(int pxs, int ys, char *data, FILE *fp)
{
	char g_string[256];

	for (; ys > 0; ys--) {
		char *p = g_string;

		fgets(g_string, sizeof(g_string), fp);

		/* FIXME: Check for errors here */
		for (int xs = pxs; xs > 0; xs--) {
			*data++ = strtoul(p, &p, 10);
		}
	}
}

static int fread_PATTERNSET(WORLD *world, FILE *fp)
{
	char g_string[256];
	char arg1[128];

	for (int i = 0; i < world->patternset_num; i++) {
		PATTERNSET *patternset = world->patternset + i;

		fgets(g_string, sizeof(g_string), fp);
		sscanf(g_string, "%s %d %d",
			&arg1,
			&patternset->xs, &patternset->ys);

		if (strcmp(arg1, "PATTERNSET") != 0) {
			DPRINTF("Error reading PATTERNSET chunk %d\n", i);

			return 0;
		}

		/* Allocate patternset data */
		patternset->data = (char *)
			calloc(patternset->ys, patternset->xs);

		fread_PATTERNSET_array(patternset->xs,
					patternset->ys,
					patternset->data, fp);
	}

	return 1;
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

	if (!fread_HEADER(fp))
		goto _exit_sub;

	if (!fread_WORLD(world, fp))
		goto _exit_sub;

	if (!fread_ROOM(world, fp))
		goto _exit_sub;

	if (!fread_ROOM_PATTERN(world, fp))
		goto _exit_sub;

	if (!fread_ROOM_OBJECT(world, fp))
		goto _exit_sub;

	if (!fread_ROOM_BGLINE(world, fp))
		goto _exit_sub;

	fread_PATTERNSET(world, fp);

_exit_sub:
	fclose(fp);
	return world;
}

void free_world(WORLD *world)
{
	int i;

	// roll through all rooms
	for (i = 0; i < world->room_num; i++) {
		ROOM *room = world->room + i;

		if (room->pattern) {
			free(room->pattern);
			room->pattern = NULL;
		}

		if (room->object) {
			free(room->object);
			room->object = NULL;
		}

		if (room->bgline) {
			free(room->bgline);
			room->bgline = NULL;
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
