/*

	SDL audio backend for Windows, *nix and Dingoo, adlib emulation

*/

#include <string.h>
#include "SDL/SDL.h"
#include "fmopl.h"
#include "m_snd.h"

#if !defined(__WIN32__) && !defined(__UNIX__)
#define FREQHZ 44100
#else // for dingoo, dingux and other handhelds
#define FREQHZ 32000
#endif
#define BUFFSMPL 4096

// external func in m_snd.c
void rad_update_frame();

int framesmpl = FREQHZ / 60;
int ym = 0;
INT16 buf[BUFFSMPL*2]; // stereo

// a flag to see if sdl audio is initialized and working
int fSDL_Running = 0;
SDL_AudioSpec audiostream;

// when compiling for dingoo, make -D__cdecl=""
void __cdecl playcallback(void *userdata, unsigned char *stream, int length)
{
	static int c, cnt = 0;

	for(c = 0; c <= BUFFSMPL - 1; c++)
	{
		if(cnt >= framesmpl)
		{
			cnt = 0;
			rad_update_frame();
		}
		YM3812UpdateOne(ym, &buf[c*2], 1);
		buf[c*2+1] = buf[c*2];
		cnt += 1;
	}

	memcpy(stream, &buf, length);

}

int LM_SND_Init()
{
	ym = YM3812Init(1, OPL2_INTERNAL_FREQ, FREQHZ);
	YM3812ResetChip(ym);

	audiostream.freq = FREQHZ;
	audiostream.format = AUDIO_S16;
	audiostream.channels = 2;
	audiostream.samples = BUFFSMPL;
	audiostream.callback = &playcallback;
	audiostream.userdata = 0;

	if(SDL_OpenAudio(&audiostream, 0) != 0) return 0;

	SDL_PauseAudio(0);
	fSDL_Running = 1;
	return 1;
}

int LM_SND_Deinit()
{
	if(fSDL_Running == 0) return 0;

	rad_stop_music();
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	return 1;
}

// value in Hz for timer
void rad_set_timer(int value)
{
	if(value < 18) value = 18;
	framesmpl = FREQHZ / value;
}

void rad_adlib_write(unsigned char adl_reg,unsigned char adl_data)
{
	YM3812Write(ym, 0, adl_reg);
	YM3812Write(ym, 1, adl_data);
}

void rad_adlib_reset()
{
	YM3812ResetChip(ym);
}


