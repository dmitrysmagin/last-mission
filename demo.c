/*

	Routines to record and replay demo mode

*/

#include <stdio.h>
#include <string.h>

#include "demo.h"
#include "random.h"
#include "video.h"
#include "input.h"
#include "engine.h"
#include "room.h"

#define DEMO_UP 0x01
#define DEMO_DOWN 0x02
#define DEMO_LEFT 0x04
#define DEMO_RIGHT 0x08
#define DEMO_SPACE 0x10
#define DEMO_ESCAPE 0x20

extern unsigned char KeyLog[]; // forward reference

unsigned char demo_frames = 0, demo_data = 0, demo_prev_data;

#if 0
unsigned char buffer[3000] = {0};
#endif
unsigned char *pBuffer = 0;

void RecordDemo()
{
	#if 0
	// init
	if (pBuffer == 0) {
		pBuffer = buffer;
		memset(buffer, 0, sizeof(buffer));
		pBuffer += 2; // two zeroes at the beginning
		demo_frames = 0;
		demo_data = 0;
		demo_prev_data = 0;
	}

	// stop recording demo and dump it to file
	if (Keys[SC_ESCAPE] == 1) {
		*pBuffer = demo_frames;
		*(pBuffer+1) = DEMO_ESCAPE;
		pBuffer += 2;
		demo_frames = 0;

		FILE *f = fopen("keylog.bin", "wb+");
		fwrite(buffer, sizeof(buffer[0]), (int)(pBuffer - buffer), f);
		fclose(f);
	
	
		return;
	}

	// no key pressed
	// check previous data
	if (Keys[SC_UP] == 0 && (demo_data & DEMO_UP) != 0)
		demo_data &= ~DEMO_UP;
	if (Keys[SC_DOWN] == 0 && (demo_data & DEMO_DOWN) != 0)
		demo_data &= ~DEMO_DOWN;
	if (Keys[SC_LEFT] == 0 && (demo_data & DEMO_LEFT) != 0)
		demo_data &= ~DEMO_LEFT;
	if (Keys[SC_RIGHT] == 0 && (demo_data & DEMO_RIGHT) != 0)
		demo_data &= ~DEMO_RIGHT;
	if (Keys[SC_SPACE] == 0 && (demo_data & DEMO_SPACE) != 0)
		demo_data &= ~DEMO_SPACE;

	// if pressed
	if (Keys[SC_UP] == 1 && (demo_data & DEMO_UP) == 0)
		demo_data |= DEMO_UP;
	if (Keys[SC_DOWN] == 1 && (demo_data & DEMO_DOWN) == 0)
		demo_data |= DEMO_DOWN;
	if (Keys[SC_LEFT] == 1 && (demo_data & DEMO_LEFT) == 0)
		demo_data |= DEMO_LEFT;
	if (Keys[SC_RIGHT] == 1 && (demo_data & DEMO_RIGHT) == 0)
		demo_data |= DEMO_RIGHT;
	if (Keys[SC_SPACE] == 1 && (demo_data & DEMO_SPACE) == 0)
		demo_data |= DEMO_SPACE;

	if (demo_prev_data == demo_data) {
		/*if (demo_frames == 255) {
			*pBuffer = demo_frames;
			*(pBuffer+1) = demo_prev_data;
			pBuffer += 2;
			demo_frames = 0;		
		} else*/
			demo_frames++;
	} else {
		*pBuffer = demo_frames;
		*(pBuffer+1) = demo_data;
		pBuffer += 2;
		demo_frames = 0;
		demo_prev_data = demo_data;
	}
	#endif
}

void ResetDemo()
{
	Randomize(0);
	pBuffer = KeyLog;
	demo_frames = *pBuffer;
	demo_data = *(pBuffer+1);
	demo_prev_data = 0;
}

int PlayDemo()
{
	GKeys[KEY_LEFT] = (demo_prev_data & DEMO_LEFT) >> 2;
	GKeys[KEY_RIGHT] = (demo_prev_data & DEMO_RIGHT) >> 3;
	GKeys[KEY_UP] = (demo_prev_data & DEMO_UP);
	GKeys[KEY_DOWN] = (demo_prev_data & DEMO_DOWN) >> 1;
	GKeys[KEY_FIRE] = (demo_prev_data & DEMO_SPACE) >> 4;
	GKeys[KEY_QUIT] = (demo_prev_data & DEMO_ESCAPE) >> 5;

	// demo ends with ESC pressed
	if ((demo_prev_data & DEMO_ESCAPE) >> 5 == 1)
		return 1;

	if (demo_frames == 0) {
		demo_prev_data = demo_data;
		pBuffer += 2;
		demo_frames = *pBuffer;
		demo_data = *(pBuffer+1);	
	} else
		demo_frames--;

	return 0;
}

unsigned char KeyLog[] =
{
    0x00, 0x00, 0x29, 0x04, 
    0x05, 0x00, 0x18, 0x01, 
    0x09, 0x00, 0x3A, 0x08, 
    0x48, 0x00, 0x11, 0x01, 
    0x02, 0x11, 0x02, 0x10, 
    0x02, 0x00, 0x19, 0x10, 
    0x03, 0x00, 0x1E, 0x10, 
    0x02, 0x00, 0x0A, 0x08, 
    0x89, 0x00, 0x0E, 0x08, 
    0x03, 0x00, 0x18, 0x10, 
    0x04, 0x00, 0x0A, 0x08, 
    0x05, 0x00, 0x08, 0x01, 
    0x06, 0x11, 0x01, 0x10, 
    0x00, 0x00, 0x10, 0x01, 
    0x18, 0x11, 0x05, 0x01, 
    0x02, 0x00, 0x21, 0x10, 
    0x05, 0x00, 0x1E, 0x02, 
    0x0C, 0x00, 0x03, 0x08, 
    0xD6, 0x0C, 0x00, 0x04, 
    0x18, 0x00, 0x0B, 0x10, 
    0x03, 0x00, 0x0B, 0x04, 
    0x2F, 0x00, 0x02, 0x08, 
    0x06, 0x09, 0x0A, 0x08, 
    0x0D, 0x0A, 0x10, 0x02, 
    0x00, 0x00, 0x1D, 0x10, 
    0x02, 0x00, 0x66, 0x01, 
    0x1E, 0x09, 0x0A, 0x01, 
    0x10, 0x09, 0x0E, 0x08, 
    0x31, 0x09, 0x16, 0x19, 
    0x01, 0x11, 0x00, 0x10, 
    0x00, 0x00, 0x1D, 0x10, 
    0x04, 0x00, 0x05, 0x01, 
    0x17, 0x11, 0x01, 0x10, 
    0x00, 0x00, 0x08, 0x08, 
    0x17, 0x0A, 0x1B, 0x08, 
    0x8C, 0x18, 0x03, 0x08, 
    0x00, 0x00, 0x04, 0x04, 
    0x0E, 0x14, 0x02, 0x04, 
    0x00, 0x00, 0x03, 0x08, 
    0x12, 0x09, 0x01, 0x01, 
    0x01, 0x11, 0x22, 0x01, 
    0x00, 0x09, 0x00, 0x08, 
    0x8B, 0x09, 0x16, 0x08, 
    0x19, 0x0A, 0x15, 0x0E, 
    0x0B, 0x06, 0x17, 0x02, 
    0x00, 0x00, 0x27, 0x10, 
    0x03, 0x00, 0x06, 0x01, 
    0x09, 0x00, 0x0B, 0x10, 
    0x04, 0x00, 0x0F, 0x01, 
    0x06, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x1C, 0x10, 
    0x04, 0x00, 0x1A, 0x10, 
    0x01, 0x00, 0x06, 0x04, 
    0x80, 0x00, 0x10, 0x01, 
    0x0D, 0x00, 0x01, 0x10, 
    0x04, 0x00, 0x10, 0x10, 
    0x04, 0x00, 0x10, 0x04, 
    0x0D, 0x05, 0x00, 0x01, 
    0x07, 0x11, 0x01, 0x15, 
    0x00, 0x14, 0x00, 0x04, 
    0x88, 0x05, 0x03, 0x04, 
    0x02, 0x14, 0x01, 0x04, 
    0x35, 0x14, 0x03, 0x04, 
    0x45, 0x00, 0x11, 0x01, 
    0x03, 0x05, 0x00, 0x04, 
    0x00, 0x14, 0x02, 0x04, 
    0x31, 0x05, 0x10, 0x15, 
    0x01, 0x14, 0x00, 0x04, 
    0x0B, 0x00, 0x07, 0x08, 
    0x12, 0x00, 0x0F, 0x10, 
    0x02, 0x00, 0x07, 0x08, 
    0x3F, 0x0C, 0x00, 0x04, 
    0x11, 0x00, 0x09, 0x01, 
    0x0C, 0x11, 0x07, 0x01, 
    0x02, 0x00, 0x0C, 0x04, 
    0x39, 0x05, 0x08, 0x04, 
    0x22, 0x00, 0x00, 0x08, 
    0x86, 0x00, 0x0B, 0x08, 
    0x02, 0x00, 0x0D, 0x01, 
    0x06, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x07, 0x01, 
    0x0C, 0x11, 0x01, 0x10, 
    0x03, 0x00, 0x14, 0x10, 
    0x01, 0x00, 0x03, 0x10, 
    0x02, 0x00, 0x16, 0x02, 
    0x0B, 0x00, 0x04, 0x08, 
    0x89, 0x00, 0x0C, 0x08, 
    0x02, 0x00, 0x0B, 0x01, 
    0x0E, 0x11, 0x02, 0x10, 
    0x02, 0x00, 0x0E, 0x10, 
    0x05, 0x00, 0x0C, 0x01, 
    0x06, 0x00, 0x01, 0x10, 
    0x05, 0x00, 0x0E, 0x02, 
    0x15, 0x00, 0x00, 0x08, 
    0x8C, 0x00, 0x0E, 0x01, 
    0x0B, 0x11, 0x01, 0x10, 
    0x04, 0x00, 0x07, 0x02, 
    0x0D, 0x00, 0x04, 0x08, 
    0x05, 0x18, 0x06, 0x08, 
    0x79, 0x00, 0x02, 0x04, 
    0x0C, 0x00, 0x51, 0x04, 
    0x84, 0x00, 0x18, 0x10, 
    0x03, 0x00, 0x07, 0x04, 
    0x05, 0x00, 0x0D, 0x08, 
    0x00, 0x09, 0x11, 0x08, 
    0x3C, 0x09, 0x15, 0x08, 
    0x23, 0x0A, 0x12, 0x02, 
    0x08, 0x06, 0x11, 0x02, 
    0x10, 0x00, 0x13, 0x10, 
    0x04, 0x00, 0x19, 0x01, 
    0x07, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x0F, 0x04, 
    0x0D, 0x05, 0x06, 0x15, 
    0x01, 0x14, 0x01, 0x04, 
    0x69, 0x00, 0x0B, 0x01, 
    0x0E, 0x11, 0x01, 0x10, 
    0x04, 0x00, 0x08, 0x01, 
    0x07, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x04, 0x04, 
    0x04, 0x05, 0x05, 0x04, 
    0x0B, 0x14, 0x04, 0x04, 
    0x05, 0x00, 0x0F, 0x01, 
    0x08, 0x00, 0x11, 0x01, 
    0x05, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x0D, 0x04, 
    0x32, 0x0C, 0x01, 0x08, 
    0x10, 0x00, 0x94, 0x01, 
    0x1B, 0x09, 0x06, 0x01, 
    0x1C, 0x09, 0x01, 0x01, 
    0x18, 0x11, 0x07, 0x01, 
    0x02, 0x00, 0x14, 0x10, 
    0x02, 0x00, 0x04, 0x01, 
    0x0B, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x06, 0x01, 
    0x07, 0x00, 0x01, 0x10, 
    0x02, 0x00, 0x06, 0x08, 
    0x27, 0x0C, 0x00, 0x04, 
    0x0E, 0x05, 0x00, 0x01, 
    0x05, 0x11, 0x01, 0x10, 
    0x00, 0x00, 0x25, 0x10, 
    0x01, 0x11, 0x02, 0x01, 
    0x05, 0x00, 0x1A, 0x10, 
    0x03, 0x00, 0x0C, 0x04, 
    0x05, 0x05, 0x0A, 0x04, 
    0x2C, 0x00, 0x00, 0x08, 
    0x0C, 0x00, 0x0B, 0x10, 
    0x02, 0x00, 0x06, 0x08, 
    0x02, 0x09, 0x16, 0x08, 
    0x3F, 0x00, 0x00, 0x02, 
    0x07, 0x06, 0x00, 0x04, 
    0x9A, 0x00, 0x0F, 0x10, 
    0x05, 0x00, 0x10, 0x01, 
    0x05, 0x11, 0x01, 0x10, 
    0x03, 0x00, 0x0B, 0x01, 
    0x16, 0x11, 0x01, 0x10, 
    0x03, 0x00, 0x07, 0x02, 
    0x1D, 0x00, 0x03, 0x04, 
    0x8C, 0x00, 0x0A, 0x10, 
    0x04, 0x00, 0x2D, 0x10, 
    0x05, 0x00, 0x11, 0x01, 
    0x08, 0x00, 0x03, 0x10, 
    0x03, 0x00, 0x2D, 0x04, 
    0x8A, 0x00, 0x0C, 0x01, 
    0x12, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x07, 0x02, 
    0x11, 0x12, 0x04, 0x02, 
    0x03, 0x00, 0x1B, 0x10, 
    0x05, 0x00, 0x2B, 0x04, 
    0x89, 0x00, 0x2A, 0x04, 
    0x22, 0x00, 0x05, 0x01, 
    0x07, 0x00, 0x00, 0x10, 
    0x03, 0x00, 0x05, 0x02, 
    0x09, 0x00, 0x0F, 0x01, 
    0x0C, 0x00, 0x02, 0x10, 
    0x03, 0x00, 0x02, 0x02, 
    0x12, 0x00, 0x04, 0x04, 
    0x61, 0x14, 0x06, 0x04, 
    0x00, 0x00, 0x1D, 0x10, 
    0x05, 0x00, 0x1C, 0x10, 
    0x05, 0x00, 0x0E, 0x04, 
    0x0A, 0x00, 0x08, 0x01, 
    0x0B, 0x00, 0x15, 0x01, 
    0x0D, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x1D, 0x01, 
    0x08, 0x11, 0x01, 0x10, 
    0x04, 0x00, 0x09, 0x02, 
    0x1C, 0x06, 0x00, 0x04, 
    0x7C, 0x00, 0x01, 0x08, 
    0x0E, 0x00, 0x57, 0x08, 
    0x7B, 0x00, 0x0E, 0x08, 
    0x02, 0x00, 0x14, 0x08, 
    0x04, 0x00, 0x0F, 0x01, 
    0x0C, 0x00, 0x00, 0x10, 
    0x05, 0x00, 0x24, 0x10, 
    0x06, 0x00, 0x0C, 0x10, 
    0x06, 0x00, 0x11, 0x10, 
    0x05, 0x00, 0x0C, 0x10, 
    0x06, 0x00, 0x16, 0x08, 
    0x07, 0x00, 0x0F, 0x01, 
    0x08, 0x00, 0x00, 0x10, 
    0x01, 0x00, 0x16, 0x02, 
    0x06, 0x00, 0x03, 0x08, 
    0x2E, 0x00, 0x02, 0x01, 
    0x0A, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x0B, 0x02, 
    0x0B, 0x00, 0x05, 0x08, 
    0x0F, 0x00, 0x02, 0x01, 
    0x14, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x08, 0x02, 
    0x19, 0x0A, 0x00, 0x08, 
    0x46, 0x00, 0x0B, 0x08, 
    0x03, 0x09, 0x05, 0x08, 
    0x15, 0x09, 0x22, 0x01, 
    0x1F, 0x00, 0x01, 0x10, 
    0x04, 0x00, 0x11, 0x01, 
    0x08, 0x00, 0x05, 0x08, 
    0x0D, 0x09, 0x07, 0x01, 
    0x03, 0x11, 0x03, 0x01, 
    0x09, 0x00, 0x06, 0x01, 
    0x03, 0x09, 0x02, 0x08, 
    0x2E, 0x00, 0x00, 0x01, 
    0x05, 0x00, 0x02, 0x10, 
    0x04, 0x00, 0x0F, 0x08, 
    0x04, 0x00, 0x07, 0x01, 
    0x05, 0x00, 0x02, 0x10, 
    0x01, 0x00, 0x14, 0x01, 
    0x16, 0x09, 0x08, 0x08, 
    0x15, 0x00, 0x02, 0x10, 
    0x03, 0x00, 0x0E, 0x08, 
    0x07, 0x18, 0x03, 0x08, 
    0x05, 0x00, 0x01, 0x01, 
    0x0C, 0x09, 0x0C, 0x08, 
    0x2A, 0x09, 0x21, 0x08, 
    0x11, 0x0A, 0x07, 0x08, 
    0x05, 0x09, 0x14, 0x01, 
    0x03, 0x00, 0x3D, 0x04, 
    0x0B, 0x00, 0x00, 0x08, 
    0x12, 0x00, 0x06, 0x08, 
    0x06, 0x00, 0x06, 0x04, 
    0x1D, 0x0C, 0x01, 0x08, 
    0x51, 0x00, 0x0F, 0x01, 
    0x0A, 0x00, 0x00, 0x10, 
    0x04, 0x00, 0x21, 0x10, 
    0x07, 0x00, 0x0A, 0x10, 
    0x06, 0x00, 0x11, 0x10, 
    0x05, 0x00, 0x0D, 0x10, 
    0x05, 0x00, 0x09, 0x10, 
    0x02, 0x00, 0x16, 0x08, 
    0x05, 0x00, 0x0A, 0x01, 
    0x05, 0x00, 0x01, 0x10, 
    0x04, 0x00, 0x08, 0x02, 
    0x09, 0x00, 0x03, 0x08, 
    0x37, 0x00, 0x04, 0x01, 
    0x09, 0x11, 0x02, 0x10, 
    0x00, 0x00, 0x0C, 0x02, 
    0x10, 0x00, 0x00, 0x08, 
    0x0F, 0x00, 0x03, 0x01, 
    0x16, 0x11, 0x01, 0x10, 
    0x03, 0x00, 0x00, 0x02, 
    0x1C, 0x0A, 0x00, 0x08, 
    0x45, 0x00, 0x0F, 0x08, 
    0x01, 0x09, 0x05, 0x08, 
    0x12, 0x09, 0x23, 0x01, 
    0x1C, 0x00, 0x03, 0x10, 
    0x01, 0x00, 0x17, 0x01, 
    0x08, 0x00, 0x02, 0x10, 
    0x02, 0x00, 0x08, 0x08, 
    0x03, 0x09, 0x0E, 0x08, 
    0x09, 0x09, 0x07, 0x08, 
    0x1C, 0x00, 0x0B, 0x10, 
    0x01, 0x00, 0x07, 0x08, 
    0x06, 0x00, 0x05, 0x01, 
    0x05, 0x00, 0x01, 0x10, 
    0x04, 0x00, 0x10, 0x01, 
    0x07, 0x09, 0x00, 0x08, 
    0x12, 0x18, 0x04, 0x08, 
    0x00, 0x09, 0x08, 0x08, 
    0x12, 0x00, 0x23, 0x01, 
    0x03, 0x11, 0x04, 0x01, 
    0x08, 0x00, 0x09, 0x08, 
    0x14, 0x09, 0x48, 0x08, 
    0x15, 0x09, 0x05, 0x08, 
    0x16, 0x0A, 0x0A, 0x08, 
    0x0E, 0x09, 0x0B, 0x08, 
    0x02, 0x18, 0x05, 0x08, 
    0x10, 0x18, 0x04, 0x08, 
    0x02, 0x00, 0x12, 0x10, 
    0x05, 0x00, 0x11, 0x10, 
    0x03, 0x00, 0x0F, 0x01, 
    0x02, 0x09, 0x0C, 0x08, 
    0x24, 0x09, 0x04, 0x08, 
    0x05, 0x09, 0x06, 0x01, 
    0x09, 0x09, 0x08, 0x08, 
    0x14, 0x0A, 0x14, 0x1A, 
    0x04, 0x18, 0x01, 0x08, 
    0x07, 0x09, 0x1E, 0x08, 
    0x19, 0x09, 0x07, 0x08, 
    0x51, 0x09, 0x10, 0x08, 
    0x22, 0x0A, 0x1A, 0x0E, 
    0x00, 0x06, 0x14, 0x02, 
    0x08, 0x00, 0x0D, 0x10, 
    0x02, 0x00, 0x0E, 0x10, 
    0x05, 0x00, 0x09, 0x10, 
    0x04, 0x00, 0x0A, 0x01, 
    0x03, 0x00, 0x13, 0x01, 
    0x01, 0x11, 0x05, 0x01, 
    0x00, 0x00, 0x23, 0x01, 
    0x05, 0x11, 0x01, 0x10, 
    0x00, 0x00, 0x07, 0x04, 
    0x16, 0x05, 0x08, 0x04, 
    0x05, 0x14, 0x05, 0x04, 
    0x1B, 0x05, 0x04, 0x04, 
    0x33, 0x00, 0x16, 0x01, 
    0x0C, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x06, 0x01, 
    0x0A, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x22, 0x01, 
    0x08, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x2A, 0x01, 
    0x07, 0x00, 0x00, 0x10, 
    0x03, 0x00, 0x28, 0x01, 
    0x07, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x0B, 0x04, 
    0x0A, 0x05, 0x15, 0x04, 
    0x15, 0x05, 0x03, 0x04, 
    0x30, 0x06, 0x10, 0x04, 
    0x16, 0x00, 0x0E, 0x01, 
    0x08, 0x11, 0x01, 0x10, 
    0x01, 0x00, 0x0E, 0x01, 
    0x02, 0x05, 0x05, 0x04, 
    0x01, 0x14, 0x02, 0x04, 
    0x84, 0x00, 0x0F, 0x01, 
    0x09, 0x00, 0x02, 0x10, 
    0x04, 0x12, 0x01, 0x02, 
    0x03, 0x06, 0x00, 0x04, 
    0x22, 0x05, 0x09, 0x15, 
    0x01, 0x14, 0x01, 0x04, 
    0x60, 0x00, 0x0A, 0x01, 
    0x05, 0x11, 0x01, 0x15, 
    0x00, 0x14, 0x00, 0x04, 
    0x07, 0x00, 0x05, 0x01, 
    0x0A, 0x00, 0x01, 0x10, 
    0x03, 0x00, 0x09, 0x01, 
    0x07, 0x11, 0x01, 0x10, 
    0x00, 0x00, 0x05, 0x01, 
    0x06, 0x00, 0x11, 0x10, 
    0x02, 0x00, 0x07, 0x04, 
    0x3B, 0x05, 0x06, 0x04, 
    0x07, 0x05, 0x02, 0x04, 
    0x26, 0x00, 0x02, 0x08, 
    0x30, 0x18, 0x04, 0x08, 
    0x59, 0x00, 0x26, 0x10, 
    0x03, 0x00, 0x2C, 0x10, 
    0x05, 0x00, 0x1D, 0x10, 
    0x07, 0x00, 0x2D, 0x10, 
    0x03, 0x00, 0x27, 0x01, 
    0x18, 0x11, 0x01, 0x10, 
    0x05, 0x00, 0x08, 0x02, 
    0x19, 0x12, 0x03, 0x10, 
    0x02, 0x00, 0x12, 0x08, 
    0x00, 0x09, 0x06, 0x08, 
    0x15, 0x0A, 0x07, 0x1A, 
    0x04, 0x12, 0x01, 0x02, 
    0x00, 0x00, 0x0D, 0x04, 
    0x04, 0x05, 0x0F, 0x04, 
    0x1F, 0x00, 0x06, 0x08, 
    0x94, 0x00, 0x18, 0x01, 
    0x08, 0x11, 0x01, 0x10, 
    0x02, 0x00, 0x17, 0x10, 
    0x04, 0x00, 0x11, 0x08, 
    0x8D, 0x00, 0x28, 0x10, 
    0x01, 0x18, 0x02, 0x08, 
    0x0B, 0x18, 0x06, 0x08, 
    0x0D, 0x18, 0x01, 0x10, 
    0x01, 0x00, 0x13, 0x08, 
    0x12, 0x00, 0x01, 0x08, 
    0x02, 0x18, 0x06, 0x08, 
    0x4D, 0x00, 0x10, 0x01, 
    0x05, 0x00, 0x02, 0x10, 
    0x03, 0x12, 0x01, 0x02, 
    0x0A, 0x0A, 0x00, 0x08, 
    0x7A, 0x00, 0x01, 0x04, 
    0x11, 0x00, 0x02, 0x10, 
    0x02, 0x00, 0x1A, 0x10, 
    0x02, 0x00, 0x24, 0x08, 
    0x1D, 0x00, 0x0A, 0x04, 
    0x07, 0x00, 0x59, 0x04, 
    0x89, 0x00, 0x16, 0x04, 
    0x01, 0x14, 0x06, 0x04, 
    0x26, 0x00, 0x00, 0x08, 
    0x0E, 0x00, 0x0D, 0x10, 
    0x05, 0x00, 0x07, 0x08, 
    0x1E, 0x0C, 0x00, 0x04, 
    0x09, 0x14, 0x03, 0x10, 
    0x01, 0x00, 0x2C, 0x04, 
    0x0A, 0x00, 0x3C, 0x10, 
    0x04, 0x00, 0x08, 0x04, 
    0x6E, 0x00, 0x22, 0x04, 
    0x0E, 0x00, 0x0C, 0x08, 
    0x00, 0x09, 0x1C, 0x08, 
    0x1A, 0x0A, 0x0C, 0x1A, 
    0x03, 0x0A, 0x08, 0x08, 
    0x18, 0x09, 0x0B, 0x01, 
    0x00, 0x00, 0x2E, 0x20, 
} ;
