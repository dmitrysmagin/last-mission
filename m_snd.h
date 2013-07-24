/*



*/

#define SF_NOTE(NOTE, OCTAVE) ((NOTE & 0x0F) | ((OCTAVE & 7) << 4))

// definitions in m_snd.c
int rad_play_music(unsigned char *ptr);
int rad_stop_music();

void rad_load_sndfx(int channel, unsigned char *p);
void rad_play_sndfx(unsigned char *p, int channel, int packednote);

// definitions in m_snd_***.c
int LM_SND_Init();
int LM_SND_Deinit();
