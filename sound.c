/*
 *
 *
 *
 */
 
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "m_core.h"
#include "sound.h"

Mix_Chunk *laser, *short_laser, *rocket_shot, *cannon_shot, *explosions[3],
	*contact, *elevator, *bonus, *move;
int moveChannel = -1, elevatorChannel = -1;

Mix_Music *music_intro, *music_game;
int musicChannel = -1;

int isInitialized = 0;

void LoadSoundFiles()
{
	if (isInitialized)
		return;

	isInitialized = 1;

	music_intro = Mix_LoadMUS("sound/music_intro.ogg");
	music_game = Mix_LoadMUS("sound/music.ogg");

	bonus = Mix_LoadWAV("sound/bonus.ogg");
	laser = Mix_LoadWAV("sound/laser.ogg");
	short_laser = Mix_LoadWAV("sound/short_laser.ogg");
	rocket_shot = Mix_LoadWAV("sound/rocket_shot.ogg");
	cannon_shot = Mix_LoadWAV("sound/cannon_shot.ogg");
	explosions[0] = Mix_LoadWAV("sound/explode0.ogg");
	explosions[1] = Mix_LoadWAV("sound/explode1.ogg");
	explosions[2] = Mix_LoadWAV("sound/explode2.ogg");
	contact = Mix_LoadWAV("sound/contact.ogg");
	move = Mix_LoadWAV("sound/move.ogg");
	elevator = Mix_LoadWAV("sound/elevator.ogg");
}

void PlaySoundEffect(int sound)
{
	switch (sound) {
	case SND_LASER_SHOOT:
		Mix_PlayChannel(-1, laser, 0);
		break;
	case SND_CANNON_SHOOT:
		Mix_PlayChannel(-1, cannon_shot, 0);
		break;
	case SND_SHORT_LASER_SHOOT:
		Mix_PlayChannel(-1, short_laser, 0);
		break;
	case SND_ROCKET_SHOOT:
		Mix_PlayChannel(-1, rocket_shot, 0);
		break;
	case SND_EXPLODE:
		Mix_PlayChannel(-1, explosions[rand() % 3], 0);
		break;
	case SND_CONTACT:
		Mix_PlayChannel(-1, contact, 0);
		break;
	case SND_BONUS:
		Mix_PlayChannel(-1, bonus, 0);
		break;
	case SND_MOVE:
		if(moveChannel == -1 || !Mix_Playing(moveChannel)) {
			moveChannel = Mix_PlayChannel(-1, move, -1);
		}
		break;
	case SND_ELEVATOR:
		if(elevatorChannel == -1 || !Mix_Playing(elevatorChannel)) {
			elevatorChannel = Mix_PlayChannel(-1, elevator, -1);
		}
		break;
	}
}

void StopSoundEffect(int sound)
{
	switch (sound) {
	case SND_MOVE:
		if(moveChannel > -1) {
			if(Mix_Playing(moveChannel)) Mix_FadeOutChannel(moveChannel, 250);
			moveChannel = -1;
		}
		break;
	case SND_ELEVATOR:
		if (elevatorChannel > -1) {
			if(Mix_Playing(elevatorChannel)) Mix_FadeOutChannel(elevatorChannel, 250);
			elevatorChannel = -1;
		}
		break;
	}
}

void PlayMusic(int music)
{
	if (musicChannel != -1) {
		Mix_HaltMusic();
		musicChannel = -1;
	}

	switch (music) {
	case MUSIC_STOP:
		break;
	case MUSIC_INTRO:
	//case MUSIC_WIN:
	//case MUSIC_LOSE:
		musicChannel = Mix_PlayMusic(music_intro, -1);
		break;
	case MUSIC_GAME:
		musicChannel = Mix_PlayMusic(music_game, -1);
		break;
	}
}

int LM_SND_Init()
{
	if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) != MIX_INIT_OGG) {
		printf("Failed to init OGG support\n");
	}

	Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024);
	LoadSoundFiles();
	return 1;
}

int LM_SND_Deinit()
{
	Mix_CloseAudio();
	Mix_Quit();
	return 1;
}
