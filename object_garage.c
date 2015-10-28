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
#include "engine.h"
#include "object.h"
#include "object_garage.h"
#include "room.h"

// Actual garage data, valid for current game.
static int garage_data[MAX_GARAGES][2];
// Garage data of the last game start. Will be restored if player is dead.
static int main_garage_data[MAX_GARAGES][2];

void GarageRestore()
{
	memcpy(garage_data, main_garage_data, sizeof(garage_data));
}

void GarageSave()
{
	memcpy(main_garage_data, garage_data, sizeof(main_garage_data));
}

void BestPositionInGarage(TSHIP *ship, int *x, int *y)
{
	int cxShip, cyShip;

	cxShip = gObj_GetWidth(ship);
	cyShip = gObj_GetHeight(ship);

	if (ship->garage == NULL) {
		// actually, should not happen.
		*x = ship->x;
		*y = ship->y;
	} else {
		TSHIP *garage = ship->garage;
		*x = garage->x + ((GARAGE_WIDTH - cxShip) >> 1);
		*y = garage->y + ((GARAGE_HEIGHT - cyShip) >> 1);
	}
}

void InitGaragesForNewGame()
{
	memset(garage_data, 0, sizeof(garage_data));

	int n = 0;
	garage_data[n  ][0] = 100;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 101;
	garage_data[n++][1] = SHIP_TYPE_MACHINE_GUN;

	garage_data[n  ][0] = 110;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 111;
	garage_data[n++][1] = SHIP_TYPE_ROCKET_LAUNCHER;

	garage_data[n  ][0] = 120;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 121;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 122;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 123;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 124;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 130;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 131;
	garage_data[n++][1] = SHIP_TYPE_OBSERVER;

	garage_data[n  ][0] = 190;
	garage_data[n++][1] = -1;

	garage_data[n  ][0] = 191;
	garage_data[n++][1] = SHIP_TYPE_BFG;

	GarageSave();
}

void SetGarageShipIndex(int garageId, int shipIndex)
{
	for (int i = 0; i < MAX_GARAGES; ++i) {
		if (garage_data[i][0] == garageId) {
			garage_data[i][1] = shipIndex;
		}
	}
}

int GarageShipIndex(int garageId)
{
	for (int i = 0; i < MAX_GARAGES; ++i) {
		if (garage_data[i][0] == garageId) {
			return garage_data[i][1];
		}
	}

	return -1;
}

void CreateGarage(TSHIP *en, int garage_id)
{
	en->i = garage_id;

	int iShip = GarageShipIndex(en->i);
	if (iShip != -1) {
		// Find which type of the ship is supposed to be here,
		// create the ship in the best position inside it.
		TSHIP *ship = gObj_CreateObject();
		ship->i = iShip;
		gObj_Constructor(ship, AI_SPARE_SHIP);
		ship->garage = en;

		switch (iShip) {
		case SHIP_TYPE_LASER:
		case SHIP_TYPE_MACHINE_GUN:
		case SHIP_TYPE_ROCKET_LAUNCHER:
			ship->max_frame = 6;
			ship->min_frame = 0;
			break;
		case SHIP_TYPE_OBSERVER:
			ship->max_frame = 3;
			ship->min_frame = 1;
			ship->cur_frame = 1;
			break;
		case SHIP_TYPE_BFG:
			ship->max_frame = 4;
			ship->min_frame = 0;
			break;
		}
		BestPositionInGarage(ship, &ship->x, &ship->y);
	}
}

int IsParked(int ship_type)
{
	for (int i = 0; i < MAX_GARAGES; ++i) {
		if (garage_data[i][0] && garage_data[i][1] == ship_type) {
			return 1;
		}
	}
	return 0;
}

int GetPlayerShipIndex()
{
	if (!IsParked(SHIP_TYPE_LASER))
		return SHIP_TYPE_LASER;

	if (!IsParked(SHIP_TYPE_MACHINE_GUN))
		return SHIP_TYPE_MACHINE_GUN;

	if (!IsParked(SHIP_TYPE_ROCKET_LAUNCHER))
		return SHIP_TYPE_ROCKET_LAUNCHER;

	if (!IsParked(SHIP_TYPE_BFG))
		return SHIP_TYPE_BFG;

	return SHIP_TYPE_OBSERVER;
}

void Update_Garage(TSHIP *gobj)
{
	TSHIP *ship = gObj_Ship();

	if (ship->ai_type == AI_EXPLOSION)
		return;

	if (GarageShipIndex(gobj->i) != -1) {
		return;
	}

	int w, h;

	w = gObj_GetWidth(ship);
	h = gObj_GetHeight(ship);

	if (!gobj->garage_inactive &&
		(ship->x >= gobj->x) &&
		(ship->x + w < gobj->x + GARAGE_WIDTH) &&
		(ship->y >= gobj->y) &&
		(ship->y + h < gobj->y + GARAGE_HEIGHT)) {
		// Player ship is inside the garage, lets
		// change the ship if possible.
		TSHIP *spare = gObj_First(2);

		for (; spare; spare = gObj_Next(spare)) {
			if (spare->ai_type == AI_SPARE_SHIP)
				break;
		}

		if (spare) {
			// Swap data of the player ship and
			// the spare ship.
			/* FIXME: Rework this ugly code */
			TSHIP tmp;
			tmp = *ship;
			*ship = *spare;
			*spare = tmp;

			/* FIXME: Restore pointer to base */
			ship->base = spare->base;

			TSHIP *garage = ship->garage;

			garage->garage_inactive = 1;

			SetGarageShipIndex(gobj->i, spare->i);
			SetGarageShipIndex(garage->i, -1);

			gObj_Constructor(ship, AI_SHIP);
			ship->garage = NULL;

			gObj_Constructor(spare, AI_SPARE_SHIP);
			spare->garage = gobj;

			// restore HP
			game->health = 3;

			PlaySoundEffect(SND_CONTACT);
		}
	} else if (gobj->garage_inactive) {
		if (!gObj_CheckOverlap(ship->x, ship->y, ship, gobj)) {
			gobj->garage_inactive = 0;
		}
	}
}

void Update_SpareShip(TSHIP *gobj)
{
	const int speed = 1;
	int x, y;
	BestPositionInGarage(gobj, &x, &y);

	if (x > gobj->x) {
		gobj->x += speed;
	} else {
		if (x < gobj->x)
			gobj->x -= speed;
	}

	if (y > gobj->y) {
		gobj->y += speed;
	} else {
		if (y < gobj->y)
			gobj->y -= speed;
	}

	int middle_frame = (gobj->max_frame + gobj->min_frame) / 2;

	if (gobj->cur_frame > gobj->min_frame && gobj->cur_frame <= middle_frame)
		--(gobj->cur_frame);
	else
		if (gobj->cur_frame > middle_frame && gobj->cur_frame < gobj->max_frame)
			++(gobj->cur_frame);
}
