/*



*/

#define SF_NOTE(NOTE, OCTAVE) ((NOTE & 0x0F) | ((OCTAVE & 7) << 4))

int LM_SND_rad_init();
int LM_SND_rad_play(unsigned char *ptr);
int LM_SND_rad_stop();
int LM_SND_rad_deinit();

//void LM_SND_rad_load_sndfx(int channel, unsigned char *p);
void LM_SND_rad_play_sndfx(unsigned char *p, int channel, int packednote);
