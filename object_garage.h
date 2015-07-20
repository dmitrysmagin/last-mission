#ifndef _OBJECT_GARAGE_H_
#define _OBJECT_GARAGE_H_

#include "enemies.h"

#define GARAGE_WIDTH 48
#define GARAGE_HEIGHT 18

void GarageSave();
void GarageRestore();
void BestPositionInGarage(TSHIP *ship, int *x, int *y);
void InitGaragesForNewGame();
void SetGarageShipIndex(int garageId, int shipIndex);
int GarageShipIndex(int garageId);
int GetPlayerShipIndex();

void CreateGarage(TSHIP *en, int garage_id);

void Update_Garage(TSHIP *gobj);
void Update_SpareShip(TSHIP *gobj);

#endif /* _OBJECT_GARAGE_H_ */