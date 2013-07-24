

#include <string.h>
#include "m_snd.h"

typedef union
{
	struct
	{
		unsigned char lo_byte;
		unsigned char hi_byte;
	};
	struct
	{
		unsigned short freq : 10;
		unsigned short octave : 3;
		unsigned short noteon : 1;
	};
} RAD_A0B0;

typedef struct
{
	unsigned char *pdata;
	unsigned short tmr_speed;
	int size;
	unsigned char pattern_jmp_f;

	char Speed;
	char SpeedCnt;
	char OrderSize;
	unsigned char *pOrderList;
	char OrderPos;

	unsigned char *pPatternList;
	unsigned char *pPatternPos;
	unsigned char CurrentLine;

	unsigned char *pInstr[31];
	unsigned char Old43[9];
	RAD_A0B0 OldA0B0[9];

	unsigned char ToneSlideSpeed[9];
	unsigned int ToneSlideFreq[9];

	char ToneSlide[9];
	char PortSlide[9];
	char VolSlide[9];


} RAD_PLAYER;

RAD_PLAYER rad;

int rad_playing = 0;

int rad_NoteFreq[12] = {0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2ae};
unsigned char rad_ChannelOffs[9] = {0x20, 0x21, 0x22, 0x28, 0x29, 0x2a, 0x30, 0x31, 0x32};

// external functions in m_snd_***.c
void rad_adlib_write(unsigned char adl_reg,unsigned char adl_data);
void rad_adlib_reset();
void rad_set_timer(int value);

void rad_load_instrument(int channel, int inst_number)
{
	int r = rad_ChannelOffs[channel];
	unsigned char *p = rad.pInstr[inst_number];

	if(p != 0)
	{
		rad.Old43[channel] = *(p + 2);
		for(int i = 0; i <= 3; i++)
		{
			rad_adlib_write(r, *(p + 1));
			rad_adlib_write(r + 3, *p);
			p += 2;
			r += 0x20;
		}
		rad_adlib_write(r + 0x40, *(p + 2));
		rad_adlib_write(r + 0x43, *(p + 1));
		rad_adlib_write(channel + 0xc0, *p);
	}
}


void rad_set_volume(int channel, int new_volume)
{
	if(new_volume > 63) new_volume = 63;
	rad.Old43[channel] = (rad.Old43[channel] & 0xc0) + (new_volume ^ 0x3f);
	rad_adlib_write(rad_ChannelOffs[channel] + 0x23, rad.Old43[channel]) ;
}

unsigned int rad_get_freq(int i)
{
	return (rad.OldA0B0[i].freq - 0x157) + rad.OldA0B0[i].octave * (0x2ae - 0x157);
}

void rad_set_freq(int i, unsigned int new_freq)
{
	rad.OldA0B0[i].freq = new_freq % (0x2ae - 0x157) + 0x157;
	rad.OldA0B0[i].octave = new_freq / (0x2ae - 0x157);
	rad_adlib_write(0xb0 + i, rad.OldA0B0[i].hi_byte);
	rad_adlib_write(0xa0 + i, rad.OldA0B0[i].lo_byte);
}

void rad_update_notes()
{
	// process portamentos
	for(int i = 0; i <= 8; i++)
	{
		if(rad.PortSlide[i] != 0) rad_set_freq(i, rad_get_freq(i) + rad.PortSlide[i]);
	}

	// process volume slides
	for(int i = 0; i <= 8; i++)
	{
		char v;
		if(rad.VolSlide[i] > 0)
		{
			v = ((rad.Old43[i] & 0x3f) ^ 0x3f) - rad.VolSlide[i];
			if(v > 63) v = 63;
			rad_set_volume(i, v);
		}
		else
		{
			v = ((rad.Old43[i] & 0x3f) ^ 0x3f) - rad.VolSlide[i];
			if(v < 0) v = 0;
			rad_set_volume(i, v);
		}
	}

	// process tone slides
	for(int i = 0; i <= 8; i++)
	{
		if(rad.ToneSlide[i] != 0)
		{
			if(rad_get_freq(i) > rad.ToneSlideFreq[i])
			{
				if(rad_get_freq(i) - rad.ToneSlideSpeed[i] < rad.ToneSlideFreq[i]) goto _jmp_0;
				rad_set_freq(i, rad_get_freq(i) - rad.ToneSlideSpeed[i]);
			}
			else
			{
				if(rad_get_freq(i) < rad.ToneSlideFreq[i])
				{
					if(rad_get_freq(i) + rad.ToneSlideSpeed[i] > rad.ToneSlideFreq[i]) goto _jmp_0;
					rad_set_freq(i, rad_get_freq(i) + rad.ToneSlideSpeed[i]);
				}
				else
				{
				_jmp_0:
					rad.ToneSlide[i] = 0;
					rad_set_freq(i, rad.ToneSlideFreq[i]);
				}
			}
		}
	}

}

void rad_playnote(unsigned char channel, unsigned int packed_value)
{
	unsigned char octave, note, instrument, effect, effect_value = 0;

	note = packed_value & 0x0f;
	octave = (packed_value >> 4) & 7;
	instrument = ((packed_value & 0xf000) >> 12) | ((packed_value & 0x80) >> 3);
	effect = (packed_value & 0x0f00) >> 8;

	if(effect != 0) effect_value = (packed_value & 0x7f0000) >> 16;

	// check if doing noteslide
	if(note != 0)
	{
		if(effect == 3)
		{
			rad.ToneSlideFreq[channel] = octave * (0x2ae - 0x157) + rad_NoteFreq[note - 1] - 0x157;

			rad.ToneSlide[channel] = 1;
			if(effect_value != 0) rad.ToneSlideSpeed[channel] = effect_value;
			return;
		}
	}

	// play note
	if(note != 0)
	{
		// first key off previous note
		rad.OldA0B0[channel].noteon = 0;
		rad_adlib_write(0xb0 + channel, rad.OldA0B0[channel].hi_byte);

		if(instrument != 0)
		{
			rad_set_volume(channel, 0);
			rad_load_instrument(channel, instrument);
		}

		if(note != 15)
		{
			rad.OldA0B0[channel].freq = rad_NoteFreq[note-1];
			rad.OldA0B0[channel].octave = octave;
			rad.OldA0B0[channel].noteon = 1;
			rad_adlib_write(0xa0 + channel, rad.OldA0B0[channel].lo_byte);
			rad_adlib_write(0xb0 + channel, rad.OldA0B0[channel].hi_byte);
		}
	}

	switch(effect)
	{
		case 1: // portamento up
			rad.PortSlide[channel] = effect_value;
			break;
		case 2: // portamento down
			rad.PortSlide[channel] = -effect_value;
			break;
		case 3: // portamento (no note given)
			if(effect_value != 0) rad.ToneSlideSpeed[channel] = effect_value;
			rad.ToneSlide[channel] = 1;
			break;
		case 5: // tone+volume slide
			rad.ToneSlide[channel] = 1; // no break after!!
		case 0x0a: // volume slide
			if(effect_value >= 50) rad.VolSlide[channel] = -(effect_value - 50); else rad.VolSlide[channel] = effect_value;
			break;
		case 0x0c: // set volume
			rad_set_volume(channel, effect_value);
			break;
		case 0x0d: // jump to line
			if(effect_value < 64) rad.pattern_jmp_f = effect_value | 0x80;
			break;
		case 0x0f: // set speed
			rad.Speed = effect_value;
			break;
	}

}

void rad_next_pattern()
{
	rad.OrderPos += 1;
	if(rad.OrderPos == rad.OrderSize) rad.OrderPos = 0;
	if((*(rad.pOrderList + rad.OrderPos) & 0x80) != 0) rad.OrderPos = *(rad.pOrderList + rad.OrderPos) & 0x7f;

	#ifdef __DINGOO__
	rad.pPatternPos = rad.pdata + *(unsigned char *)(rad.pPatternList + *(rad.pOrderList + rad.OrderPos) * 2) +
								  (*(unsigned char *)(rad.pPatternList + *(rad.pOrderList + rad.OrderPos) * 2 + 1) << 8);
	#else
	// on Dingoo this will hang due to unaligned data read
	rad.pPatternPos = rad.pdata + *(unsigned short *)(rad.pPatternList + *(rad.pOrderList + rad.OrderPos) * 2);
	#endif
}

void rad_update_frame()
{

	unsigned char *p = rad.pPatternPos;
	unsigned char ch;

	// if not playing - don't update anything
	if(rad_playing == 0) return;

	if(rad.SpeedCnt > 0) {rad.SpeedCnt -= 1; goto _jmp_update;}
	// switch off any effects
	for(int i = 0; i <= 8; i++)
	{
		rad.ToneSlide[i] = 0;
		rad.VolSlide[i] = 0;
		rad.PortSlide[i] = 0;
	}

	if(p != 0)
	{
		//rad.pPatternPos must be set already
		if((*p & 0x7f) == rad.CurrentLine)
		{
			if((*p & 0x80) != 0) rad.pPatternPos = 0; // mark the rest of pattern as blank
			p += 1; // move to first channel
			do {
				ch = *p;
				// play channels
				#ifdef __DINGOO__
				rad_playnote(ch & 0x7f, *(p+1) | (*(p+2) << 8) | (*(p+3) << 16) | (*(p+4) << 24));
				#else
				// on Dingoo this will hang due to unaligned data read
				rad_playnote(ch & 0x7f, *(unsigned int *)(p+1));
				#endif

				if((*(p + 2) & 0x0f) == 0) p += 3; else p += 4;

				// patternbreak command
				if((rad.pattern_jmp_f & 0x80) != 0)
				{
					rad.SpeedCnt = rad.Speed - 1;
					rad.CurrentLine = rad.pattern_jmp_f & 0x7f;
					rad_next_pattern();
					p = rad.pPatternPos;
					while((*p & 0x7f) < (rad.pattern_jmp_f & 0x7f))
					{
						if((*p & 0x80) != 0)
						{
							rad.pattern_jmp_f = 0;
							rad.pPatternPos = p;
							goto _jmp_update;
						}
						p += 1;
						while((*p & 0x80) == 0)
						{
							if((*(p + 2) & 0x0f) == 0) p += 3; else p += 4;
						}
					}
					rad.pattern_jmp_f = 0;
					rad.pPatternPos = p;
					goto _jmp_update;
				}
			} while((ch & 0x80) == 0);
			rad.pPatternPos = p;
		}
	}

	// update pointers
	rad.SpeedCnt = rad.Speed - 1;
	rad.CurrentLine += 1;
	if(rad.CurrentLine >= 64) {rad.CurrentLine = 0; rad_next_pattern();}

_jmp_update:
	rad_update_notes();

}

int rad_play_music(unsigned char *ptune)
{

	if(rad_playing == 1) rad_stop_music();

	#ifdef __DINGOO__
	#else
	// on Dingoo this will hang due to unaligned data read
	if(*(unsigned int *)ptune != 0x20444152) return 0;
	#endif

	memset(&rad, 0, sizeof(rad));
	memset(&rad.ToneSlideSpeed[0], 1, sizeof(rad.ToneSlideSpeed));

	rad.pdata = ptune;

	ptune += 17;
	rad.Speed = *ptune & 0x3f;
	if((*ptune & 0x60) == 0) rad_set_timer(50); else rad_set_timer(18);

	if((*ptune & 0x80) != 0)
	{
		ptune += 1;
		while(*ptune != 0) ptune +=1;
	}

	ptune += 1;

	while(*ptune != 0)
	{
		rad.pInstr[*ptune] = ptune + 1;
		ptune += 12;
	}

	ptune += 1;

	rad.OrderSize = *ptune;
	rad.pOrderList = ptune + 1;
	ptune += *ptune + 1;

	rad.pPatternList = ptune;

	#ifdef __DINGOO__
	rad.pPatternPos = rad.pdata + *(unsigned char *)(*rad.pOrderList * 2 + rad.pPatternList) +
								 (*(unsigned char *)(*rad.pOrderList * 2 + rad.pPatternList + 1) << 8);

	#else
	// on Dingoo this will hang due to unaligned data read
	rad.pPatternPos = rad.pdata + *(unsigned short *)(*rad.pOrderList * 2 + rad.pPatternList);
	#endif

	//rad.OrderPos = 0;
	//rad.SpeedCnt = 0;
	//rad.CurrentLine = 0;
	rad_playing = 1;
	return 1;
}

int rad_stop_music()
{
	rad_playing = 0;

	for(int i = 0; i < 9; i++)
	{
		rad_playnote(i, 15);
		rad_set_volume(i, 0);
	}

	rad_adlib_reset();

	return 1;
}

//
// simple sound effects
//

// load effect in hsc .ins format
void rad_load_sndfx(int channel, unsigned char *p)
{

}

void rad_play_sndfx(unsigned char *p, int channel, int packednote)
{
	rad.pInstr[*p] = p + 1;
	rad_load_instrument(channel, *p);
	rad_playnote(channel, packednote);
}














