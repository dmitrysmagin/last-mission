#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "data.h"
#include "demo.h"
#include "random.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "sprites.h"
#include "engine.h"
#include "engine-scrolling.h"
#include "object.h"
#include "object_enemy.h"
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

void CreateGarage(TSHIP *en, int garage_id, int home_room)
{
    en->i = garage_id;
    en->garage_id = garage_id;
    en->parked_ship = GarageShipIndex(garage_id); // seed from garage_data

    int iShip = en->parked_ship;
    if (iShip != -1) {
        // Find which type of the ship is supposed to be here,
        // create the ship in the best position inside it.
        TSHIP *ship = gObj_CreateObject();
        ship->i = iShip;
        gObj_Constructor(ship, AI_SPARE_SHIP);
        ship->garage = en;
        ship->home_room = home_room;
        ship->home_gx = en->home_gx;
        ship->home_gy = en->home_gy;
        ship->world_x = en->world_x;
        ship->world_y = en->world_y;

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

    if (ship == NULL)
        return;

    if (ship->ai_type == AI_EXPLOSION)
        return;

    // check if this garage already has a ship parked
    int parked = IS_SCROLLING_ENGINE() ? gobj->parked_ship : GarageShipIndex(gobj->garage_id);
    if (parked != -1 && parked != -2) {
        return;
    }

    int w, h;

    w = gObj_GetWidth(ship);
    h = gObj_GetHeight(ship);

    int ship_x = ship->x;
    int ship_y = ship->y;
    int gar_x = gobj->x;
    int gar_y = gobj->y;
    if (IS_SCROLLING_ENGINE()) {
        int vx, vy;
        Scrolling_GetViewport(&vx, &vy);
        ship_x = ship->x - vx;
        ship_y = ship->y - vy;
        gar_x = gobj->x - vx;
        gar_y = gobj->y - vy;
    }

    if (!gobj->garage_inactive &&
        (ship_x >= gar_x) &&
        (ship_x + w < gar_x + GARAGE_WIDTH) &&
        (ship_y >= gar_y) &&
        (ship_y + h < gar_y + GARAGE_HEIGHT)) {
        // Player ship is inside the garage, lets
        // change the ship if possible.
        TSHIP *spare = gObj_First();

        for (; spare; spare = gObj_Next(spare)) {
            if (spare->ai_type != AI_SPARE_SHIP)
                continue;
            // in scrolling mode, only swap with spare in the same room
            if (IS_SCROLLING_ENGINE() && spare->home_room != gobj->home_room)
                continue;
            break;
        }

        if (spare) {
            /* Swap AI_SHIP and AI_SPARE_SHIP */
            TSHIP *garage = spare->garage;

            garage->garage_inactive = 1;

            if (IS_SCROLLING_ENGINE()) {
                // per-object parked ship — don't touch global garage_data
                gobj->parked_ship = ship->i;
                garage->parked_ship = -1;
            } else {
                SetGarageShipIndex(gobj->garage_id, ship->i);
                SetGarageShipIndex(garage->garage_id, -1);
            }

            // spare becomes the new player ship
            gObj_Constructor(spare, AI_SHIP);
            spare->garage = NULL;
            spare->base = ship->base;
            spare->smoke = NULL;
            if (spare->base) spare->base->parent = spare;
            // free to roam — not clamped to any room
            spare->home_room = 0;
            spare->home_gx = 0;
            spare->home_gy = 0;
            spare->scroll_active = 1;

            // old ship becomes the new spare
            gObj_Constructor(ship, AI_SPARE_SHIP);
            ship->garage = gobj;
            ship->base = NULL;
            ship->scroll_active = 0;
            ship->home_room = gobj->home_room;
            ship->home_gx = gobj->home_gx;
            ship->home_gy = gobj->home_gy;

            // restore HP
            game->health = 3;
            Destroy_Smoke(ship);

            PlaySoundEffect(SND_CONTACT);
        }
    } else if (gobj->garage_inactive) {
        // manual overlap check using viewport-adjusted coords
        int sx2 = ship_x + w;
        int sy2 = ship_y + h;
        int gx2 = gar_x + GARAGE_WIDTH;
        int gy2 = gar_y + GARAGE_HEIGHT;
        if (ship_x >= gx2 || sx2 <= gar_x || ship_y >= gy2 || sy2 <= gar_y) {
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
