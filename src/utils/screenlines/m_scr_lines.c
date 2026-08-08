#include "../../sprites.h"
#include "m_screenlines_lvl0.c"
#include "m_screenlines_lvl1.c"
#include "m_screenlines_lvl2.c"
#include "m_screenlines_lvl3.c"
#include "m_screenlines_lvl4.c"
#include "m_screenlines_lvl5.c"
#include "m_screenlines_lvl6.c"
#include "m_screenlines_lvl7.c"
#include "m_screenlines_lvl8.c"
#include "m_screenlines_lvl9.c"
#include "m_screenlines_lvla.c"

short screenlines_0[] = {
	0
};

short screenlines_70[] = {
	0
};

short screenlines_71[] = {
	0
};

short screenlines_72[] = {
	0
};

short screenlines_73[] = {
	0
};

short screenlines_74[] = {
	0
};

short screenlines_75[] = {
	0
};

short screenlines_76[] = {
	0
};

short screenlines_77[] = {
	0
};

short screenlines_78[] = {
	0
};

short screenlines_79[] = {
	0
};

short screenlines_80[] = {
	0
};

short screenlines_81[] = {
	0
};

short screenlines_82[] = {
	0
};

short screenlines_83[] = {
	0
};

short screenlines_84[] = {
	0
};

short screenlines_85[] = {
	0
};

short screenlines_86[] = {
	0
};

short screenlines_87[] = {
	0
};

short screenlines_88[] = {
	0
};

short screenlines_89[] = {
	0
};

short screenlines_90[] = {
	0
};

short screenlines_91[] = {
	0
};

short screenlines_92[] = {
	0
};

short *SCREENLINES[NUM_SCREENS] = {
	screenlines_0,
	// Level 0
	screenlines_lvl00, screenlines_lvl01, screenlines_lvl02, screenlines_lvl03, screenlines_lvl04, screenlines_lvl05, screenlines_lvl06, 
	// Level 1
	screenlines_lvl10, screenlines_lvl11, screenlines_lvl12, screenlines_lvl13, screenlines_lvl14, screenlines_lvl15, screenlines_lvl16, 
	// Level 2
	screenlines_lvl20, screenlines_lvl21, screenlines_lvl22, screenlines_lvl23, screenlines_lvl24, screenlines_lvl25, screenlines_lvl26, 
	// Level 3
	screenlines_lvl30, screenlines_lvl31, screenlines_lvl32, screenlines_lvl33, screenlines_lvl34, screenlines_lvl35, screenlines_lvl36, 
	// Level 4
	screenlines_lvl40, screenlines_lvl41, screenlines_lvl42, screenlines_lvl43, screenlines_lvl44, screenlines_lvl45, screenlines_lvl46, 
	// Level 5
	screenlines_lvl50, screenlines_lvl51, screenlines_lvl52, screenlines_lvl53, screenlines_lvl54, screenlines_lvl55, screenlines_lvl56, 
	// Level 6
	screenlines_lvl60, screenlines_lvl61, screenlines_lvl62, screenlines_lvl63, screenlines_lvl64, screenlines_lvl65, screenlines_lvl66, 
	// Level 7
	screenlines_lvl70, screenlines_lvl71, screenlines_lvl72, screenlines_lvl73, screenlines_lvl74,
	// Level 8
	screenlines_lvl80, screenlines_lvl81, screenlines_lvl82, screenlines_lvl83, screenlines_lvl84, screenlines_lvl85, screenlines_lvl86, 
	// Level 9
	screenlines_lvl90, screenlines_lvl91, screenlines_lvl92, screenlines_lvl93, screenlines_lvl94, screenlines_lvl95, screenlines_lvl96, 
	// Level 10
	screenlines_lvla0, 
	
	screenlines_70, screenlines_71, screenlines_72, screenlines_73, screenlines_74, 
	screenlines_75, screenlines_76, screenlines_77, screenlines_78, screenlines_79, 
	screenlines_80, screenlines_81, screenlines_82, screenlines_83, screenlines_84, 
	screenlines_85, screenlines_86, screenlines_87, screenlines_88, screenlines_89, 
	screenlines_90, screenlines_91,

	// Hidden level
	screenlines_lvl00, 
	screenlines_lvl00, screenlines_lvl01, screenlines_lvl02, screenlines_lvl03, screenlines_lvl04, screenlines_lvl05, screenlines_lvl06, screenlines_lvl07
};

#define NumBackgrounds 13

typedef struct {
	unsigned int background;
	unsigned int shadow;
	unsigned int line_light;
	unsigned int line_shadow;
} ScreenDrawInfo;

static ScreenDrawInfo data[NumBackgrounds] = {
	{ RGB( 56, 56, 56), RGB( 32, 32, 32), RGB( 44, 64, 64), RGB( 44, 52, 64) },
	{ RGB(113,  0,  0), RGB( 64,  0,  0), RGB(113, 28,  0), RGB(  0,  0,  0) },
	{ RGB( 56, 56,113), RGB( 32, 32, 64), RGB( 56, 68,113), RGB( 44, 52, 64) },
	{ RGB(113, 89, 80), RGB( 64, 48, 44), RGB(113,105, 80), RGB(113, 80, 80) },
	{ RGB(113, 56,  0), RGB( 64, 32,  0), RGB(113, 85,  0), RGB(113,  0, 28) },
	{ RGB(  0, 56,113), RGB(  0, 32, 64), RGB( 56, 85,113), RGB( 44, 52, 64) },
	{ RGB( 80, 80,113), RGB( 44, 44, 64), RGB( 80, 97,113), RGB( 44, 64, 64) },
	{ RGB( 56, 68,113), RGB( 32, 40, 64), RGB( 56, 85,113), RGB( 44, 52, 64) },
	{ RGB(113, 56,  0), RGB( 64, 32,  0), RGB(113, 85,  0), RGB(113, 28,  0) },
	{ RGB(  0,113, 56), RGB(  0, 64, 32), RGB(  0,  0,  0), RGB( 32, 64, 56) },
	{ RGB(113, 28,  0), RGB( 64, 16,  0), RGB(113, 85,  0), RGB(113,  0, 28) },
	{ 0, 0 },
	{ RGB( 44, 44, 44), RGB( 32, 32, 32), RGB( 44, 64, 64), RGB( 44, 52, 64) },
};

ScreenDrawInfo *GetScreenDrawInfo(int screen)
{

	static const int screens[NumBackgrounds] = {
		8, 15, 22, 29, 36, 43, 50, 55, 62, 69, 70, 92, 999
	};

	for (int i = 0; i < NumBackgrounds; ++i) {
		if (screens[i] > screen)
			return (ScreenDrawInfo*)data + i;
	}

	return (ScreenDrawInfo*)data;
}
