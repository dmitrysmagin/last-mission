#ifndef _OBJECT_BFG_H_
#define _OBJECT_BFG_H_

#include "object.h"

void DoBFG(TSHIP *ship);
void CleanupBfg();
void BlitBfg();
void Update_BfgShot(TSHIP *gobj);

#endif /* _OBJECT_BFG_H_ */