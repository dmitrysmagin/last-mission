/*
 * Emulation of the assembler random routine used in the original Last Mission
 */

#include "random.h"

int randseed = 0x342a;

typedef union
{
	int ax;
	short al;
	short ah;
} A;
typedef union
{
	int bx;
	short bl;
	short bh;
} B;
typedef union
{
	int dx;
	short dl;
	short dh;
} D;

int RandomInt()
{
	A a;
	B b;
	D d;

	b.bx = 0;

	d.dx = randseed;
	b.bh = d.dl;
	b.bl = -3;
	a.ah = d.dh;

	b.bx -= d.dx;

	if(b.bx < 0) a.ah -= 1;

	b.bx -= d.dx;

	if(b.bx < 0) a.ah -= 1;

	d.dl = a.ah;
	d.dh = 0;

	b.bx -= d.dx;

	if(b.bx < 0) b.bx += 1;

	randseed = b.bx;

	return randseed;

}

// Set initial seed value, if 0 set to default 0x342a cause it's needed for demo mode
void Randomize(int seed)
{
	randseed = (seed == 0 ? 0x342a : seed);
}


