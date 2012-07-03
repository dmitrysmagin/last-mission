/*

	WATCOM PMODEW DOS32 video backend, uses direct video memory access

*/

#include <string.h>
#include <dos.h>
#include <time.h>
#include "m_gfx.h"

#define SC_ESCAPE	0x01

unsigned char Keys[128];
unsigned char ScreenBuffer[320*200];

void (__interrupt __far *prev_int_09)();

void __interrupt __far _kb_handler();
void LM_GFX_Init();
void LM_GFX_Deinit();

void LM_ResetKeys()
{
	memset(&Keys[0], 0, 128);
}

int LM_AnyKey()
{
	for(int i = 0; i < 127; i++)
	{
		if(Keys[i] == 1) return 1;
	}
	return 0;
}

void LM_Sleep(int sleep_time)
{
	delay(sleep_time);
}

int LM_Init(unsigned char **pScreenBuffer)
{
	prev_int_09 = _dos_getvect(9);
	_dos_setvect(9, _kb_handler);
	LM_GFX_Init();
	*pScreenBuffer = &ScreenBuffer[0];
	return 1;
}


void LM_Deinit()
{
	_dos_setvect(9, prev_int_09);
	LM_GFX_Deinit();
}

int LM_Timer()
{
	return (int)clock();
}

// watcom implicitly restores ds by calling __GETDS
void __interrupt __far _kb_handler()
{
	char scan, status;

	_asm {
		in al, 0x60
		mov scan, al
		in al, 0x61
		mov status, al
		or al, 0x80
		out 0x61, al
		mov al, status
		out 0x61, al
		mov al, 0x20
		out 0x20, al
	}

	// if demo mode - map any key to escape

	if(scan & 0x80) { // released
		Keys[scan & 0x7F] = 0;
	} else { // pressed
		Keys[scan] = 1;
	}

}


char LM_PollEvents()
{
	if(Keys[SC_ESCAPE]) return 1;
	return 0;
}

void LM_GFX_Init()
{
	_asm {
		mov eax, 0x13
		int 0x10
	}
	memset(&ScreenBuffer[0], 0, 320*200);

	// fix for color 255 - it should be black
	// aside from this, the standard hardware palette is used
	_asm {
		mov edx, 0x3c8
		mov al, 255
		out dx, al
		xor al, al
		inc edx
		out dx, al
		out dx, al
		out dx, al
	}
}

void LM_GFX_Deinit()
{
	_asm {
		mov eax, 0x3
		int 0x10
	}
}

void LM_GFX_Flip(unsigned char *p)
{
	memcpy((void *)(0xA0000), &ScreenBuffer[0], 320*200);
}

void LM_GFX_WaitVSync()
{
	_asm {
		mov dx,3DAh
	l1:
		in al, dx
		and al,08h
		jnz l1
	l2:
		in al, dx
		and al,08h
		jz  l2
	}

}

void LM_GFX_SetScale(int param)
{
}

