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

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Big-endian host not supported for .dat files, FIX LATER
#endif

typedef struct {
	char id[4];
	int size;
	int version;
	int chunks;
} HEADER;

typedef struct {
	char id[4];
	int size; // size of subsequent data
} CHUNK;


static HEADER header		= {{'L', 'A', 'S', 'T'}, sizeof(int) * 2, 0x0001, 2};
static CHUNK world_chunk	= {{'W', 'R', 'L', 'D'}, 0};
static CHUNK room_chunk		= {{'R', 'O', 'O', 'M'}, 0};
static CHUNK pattern_chunk	= {{'P', 'T', 'R', 'N'}, 0};
static CHUNK object_chunk	= {{'O', 'B', 'J', 'T'}, 0};
static CHUNK bgline_chunk	= {{'B', 'G', 'L', 'N'}, 0};
static CHUNK patternset_chunk	= {{'P', 'T', 'S', 'T'}, 0};
static CHUNK chunk;

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

		sprintf(g_string, "ROOM %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
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

		for (int j = 0; j < room->pattern_num; j++) {
			sprintf(g_string, "PATTERN %d %d %d \n",
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
	int i;

	for (i = 0; i < world->room_num; i++) {
		object_chunk.size = sizeof(OBJECT) * (world->room + i)->object_num;
		fwrite(&object_chunk, 1, sizeof(CHUNK), fp);
		fwrite((world->room + i)->object, 1, object_chunk.size, fp);
	}
}

static void fread_ROOM_OBJECT(WORLD *world, FILE *fp)
{
}

static void fwrite_ROOM_BGLINE(WORLD *world, FILE *fp)
{
	int i;

	for (i = 0; i < world->room_num; i++) {
		bgline_chunk.size = sizeof(BGLINE) * (world->room + i)->bg_num;
		fwrite(&bgline_chunk, 1, sizeof(CHUNK), fp);
		fwrite((world->room + i)->bgline, 1, bgline_chunk.size, fp);
	}
}

static void fread_ROOM_BGLINE(WORLD *world, FILE *fp)
{
}


static void fwrite_PATTERNSET(WORLD *world, FILE *fp)
{
	int i;

	for (i = 0; i < world->patternset_num; i++) {
		patternset_chunk.size = (world->patternset + i)->xs * 
					(world->patternset + i)->ys + 2 * sizeof(int);
		fwrite(&patternset_chunk, 1, sizeof(CHUNK), fp);
		fwrite(&(world->patternset + i)->xs, 1, sizeof(int), fp);
		fwrite(&(world->patternset + i)->ys, 1, sizeof(int), fp);
		fwrite((world->patternset + i)->data, 1, patternset_chunk.size - 2 * sizeof(int), fp);
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

	// write header
	fwrite_HEADER(fp);

	// write WORLD chunk
	fwrite_WORLD(world, fp);

	// write ROOM chunks
	fwrite_ROOM(world, fp);

	// write PATTERN chunks for each room
	fwrite_ROOM_PATTERN(world, fp);
#if 0
	// write OBJECT chunks for each room
	fwrite_ROOM_OBJECT(world, fp);

	// write BGLINE chunks for each room
	fwrite_ROOM_BGLINE(world, fp);

	// write PATTERNSETs for world
	fwrite_PATTERNSET(world, fp);
#endif
	fclose(fp);
}


/*
 *  Load data from .dat file allocating all pointers
 */

#define CHUNK_LAST (!memcmp(chunk.id, header.id, sizeof(chunk.id)))
#define CHUNK_WRLD (!memcmp(chunk.id, world_chunk.id, sizeof(chunk.id)))
#define CHUNK_ROOM (!memcmp(chunk.id, room_chunk.id, sizeof(chunk.id)))
#define CHUNK_PTRN (!memcmp(chunk.id, pattern_chunk.id, sizeof(chunk.id)))
#define CHUNK_OBJT (!memcmp(chunk.id, object_chunk.id, sizeof(chunk.id)))
#define CHUNK_BGLN (!memcmp(chunk.id, bgline_chunk.id, sizeof(chunk.id)))
#define CHUNK_PTST (!memcmp(chunk.id, patternset_chunk.id, sizeof(chunk.id)))

WORLD *load_world(char *name)
{
#if 0
	WORLD *world;
	FILE *fp;
	size_t res;

	world = (WORLD *)calloc(1, sizeof(WORLD));

	fp = fopen(name, "rb");

	while (1) {
		res = fread(&chunk, 1, sizeof(chunk), fp);

		if (feof(fp))
			break;

		if (CHUNK_LAST)
			goto __continue;

		else if (CHUNK_WRLD)
			fread_WORLD(world, fp);

		else if (CHUNK_ROOM)
			fread_ROOM(world, fp);

		else if (CHUNK_PTRN)
			fread_ROOM_PATTERN(world, fp);

		else if (CHUNK_OBJT)
			fread_ROOM_OBJECT(world, fp);

		else if (CHUNK_BGLN)
			fread_ROOM_BGLINE(world, fp);

		else if (CHUNK_PTST)
			fread_PATTERNSET(world, fp);

		else {
		__continue:
			fseek(fp, chunk.size, SEEK_CUR);
		}
	}

	(void)res; /* silence warning */

	fclose(fp);
	return world;
#endif
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
