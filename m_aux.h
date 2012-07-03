/*

	m_aux.h 

*/


void word2string(unsigned int value, char *buffer);
void Int2ZString(int digit, int num_of_digits, char *buffer);
unsigned char AdjustAscii(unsigned char a);
void PutGeneric(int x, int y, int xSize, int ySize, unsigned char *p);
void PutBlank(int x, int y, unsigned char *p);
void PutSprite(int x, int y,unsigned char *p);
void PutTile(int x, int y, unsigned char *p);
void PutLetter(int x, int y, unsigned char a);
void PutString(int x, int y, char *p);
void PutStream(int x, int y, unsigned char *p);
void UnpackLevel();
void BlitLevel();
void BlitBackground();
int RandomInt();
void Randomize(int seed);
