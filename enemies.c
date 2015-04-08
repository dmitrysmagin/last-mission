#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "m_data.h"
#include "demo.h"
#include "random.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "enemies.h"
#include "engine.h"
#include "room.h"

TSHIP Ships[SHIPS_NUMBER];

TSHIP *gObj_Ship() { return &Ships[0]; }
TSHIP *gObj_Base() { return &Ships[1]; }

/* FIXME: Later remove the first parameter */
TSHIP *gObj_First(int i, int *iterator)
{
	*iterator = i; /* should be the start of gobj list */

	for (; *iterator < SHIPS_NUMBER; (*iterator)++) {
		if (Ships[*iterator].state == SH_ACTIVE)
			return &Ships[*iterator];
	}

	return NULL;
}

TSHIP *gObj_Next(int *iterator)
{
	for (; ++(*iterator) < SHIPS_NUMBER; ) {
		if (Ships[*iterator].state == SH_ACTIVE)
			return &Ships[*iterator];
	}

	return NULL;
}

static int GetFreeEnemyIndex()
{
	for (int i = 2; i < SHIPS_NUMBER; i++) {
		if (Ships[i].state == SH_DEAD)
			return i; // and ai_type should be zero!
	}

	return SHIPS_NUMBER - 1;
}

TSHIP *gObj_CreateObject()
{
	TSHIP *ship = &Ships[GetFreeEnemyIndex()];
	memset(ship, 0, sizeof(TSHIP));

	return ship;
}

void gObj_DestroyObject(TSHIP *i)
{
}
