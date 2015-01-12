

extern unsigned char Keys[128];
extern unsigned char *pScreenBuffer;

void LM_ResetKeys();
int LM_AnyKey();
int LM_Init(unsigned char **pScreenBuffer);
void LM_Deinit();
char LM_PollEvents();
void LM_GFX_Flip(unsigned char *p);
void LM_GFX_SetScale(int scale);
