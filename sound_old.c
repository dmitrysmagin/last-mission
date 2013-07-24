
#include "m_core.h"
#include "m_snd.h"
#include "m_snd_data.h"


// Default sound and music implementation using LM_SND.

void PlaySoundEffect(int sound)
{
	switch (sound)
	{
	case SND_LASER_SHOOT:
		// Not yet implemented.
		break;
	case SND_EXPLODE:
		rad_play_sndfx(rad_sndfx1, 0, SF_NOTE(2, 4));
		break;
	case SND_CONTACT:
		rad_play_sndfx(rad_sndfx2, 1, SF_NOTE(1, 7));
		break;
	case SND_MOVE:
		// Not yet implemented.
		break;
	}
}

void StopSoundEffect(int param)
{
}

void PlayMusic(int music)
{
	switch (music)
	{
	case MUSIC_STOP:
	case MUSIC_GAME:
		rad_stop_music();
		break;
	case MUSIC_INTRO:
		rad_play_music(rad_tune);
		break;
	//case MUSIC_WIN:
	//case MUSIC_LOSE:
		// Nothing yet.
		break;
	}
}
