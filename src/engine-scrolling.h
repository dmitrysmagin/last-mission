#ifndef _ENGINE_SCROLLING_H_
#define _ENGINE_SCROLLING_H_

struct ROOMCONTAINER;
struct ROOMCONTAINER *Scrolling_GetContainerForWorld(int wx, int wy);
int Scrolling_GetTileAtWorld(int wx, int wy);
void Scrolling_SetTileAtWorld(int wx, int wy, int tile);
void Scrolling_GetViewport(int *x, int *y);
void Scrolling_MarkTileDestroyed(int wx, int wy);
int Scrolling_IsTileDestroyed(int wx, int wy);

void Scrolling_Init(void);
void Scrolling_Shutdown(void);
void DoScrolling(void);

#endif
