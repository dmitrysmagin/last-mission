

extern unsigned char *pScreenBuffer;

int LM_Init(unsigned char **pScreenBuffer);
void LM_Deinit();
void LM_GFX_Flip(unsigned char *p);
void LM_GFX_SetScale(int scale);
