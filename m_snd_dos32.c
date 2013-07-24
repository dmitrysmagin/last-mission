/*

	WATCOM PMODEW DOS32 audio backend, uses real adlib/opl2/opl3

*/

#include <dos.h>
#include "m_snd.h"

int clock_value = 0x1234BE / 0x12;
int clock_ticks = 0;
void (__interrupt __far *prev_int_08)();

void __interrupt __far _int08_handler();
void rad_update_frame();
void rad_adlib_reset();
void rad_adlib_write(unsigned char adl_reg,unsigned char adl_data);

int LM_SND_Init()
{
	rad_adlib_reset();
	rad_adlib_write(0x01, 0x20);
	rad_adlib_write(0x08, 0x00);
	rad_adlib_write(0xBD, 0x00);
	prev_int_08 = _dos_getvect(8);
	_dos_setvect(8, _int08_handler);

	return 1;
}

int LM_SND_Deinit()
{
	_dos_setvect(8, prev_int_08);
	rad_adlib_reset();
	return 1;
}

void rad_set_timer(int value)
{
	if(value < 18) value = 18;
	_asm {
		mov eax, 0x1234BE
		xor edx, edx
		mov ecx, value
		div ecx
		mov clock_value, eax
		mov ecx, eax
		mov	al,0B6h
		out	43h,al
		mov	al,cl
		out	40h,al
		mov	al,ch
		out	40h,al
	}
}

void rad_adlib_write(unsigned char adl_reg,unsigned char adl_data)
{
	_asm {
		mov	dx, 0x388
		mov al, [adl_reg]
		out	dx, al
		in	al, dx
		in	al, dx
		in	al, dx
		in	al, dx
		in	al, dx
		in	al, dx
		inc	dx

		mov	al, [adl_data]
		out	dx, al
		dec	edx
		mov	ah, 16h
loc_0_2353:
		in	al, dx
		dec	ah
		jnz	loc_0_2353
	}
}

void rad_adlib_reset()
{

	for(int r = 0x40; r < 0x56; r++) rad_adlib_write(r, 0x7f);
	for(int r = 0xB0; r < 0xB9; r++) rad_adlib_write(r, 0);

}

void __interrupt __far _int08_handler()
{
	rad_update_frame();
	if(clock_value == (0x1234BE / 0x12)) goto _call_old; // if normal timer
	clock_ticks += clock_value;
	if(clock_ticks < 0x10000)
	{
		_asm {
			mov	al,20h
			out	20h,al
		}
		return;
	}
_call_old:
	clock_ticks -= 0x10000;
	(*prev_int_08)();
}
