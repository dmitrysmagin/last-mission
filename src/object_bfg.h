#ifndef _OBJECT_BFG_H_
#define _OBJECT_BFG_H_

#include "object.h"

typedef struct SDL_Surface SDL_Surface;

void DoBFG(TSHIP *ship);
void CleanupBfg();
void BlitBfg(SDL_Surface *surface);
void Update_BfgShot(TSHIP *gobj);

#endif /* _OBJECT_BFG_H_ */
