/*

	MINGW WINMM audio backend, uses adlib emulation

*/

#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "fmopl.h"
#include "m_snd.h"

#define FREQHZ 44100
#define BUFFSMPL 2048

// external func in m_snd.c
void rad_update_frame();

int framesmpl = FREQHZ / 60;
int ym = 0;
short buf[BUFFSMPL];

#ifndef WAVE_MAPPER
#define WAVE_MAPPER (-1)
#endif

#define	REPLAY_CHANNELS		 1
#define	REPLAY_RATE			 44100
#define	REPLAY_DEPTH		 16
#define	REPLAY_SAMPLELEN	 (REPLAY_DEPTH*REPLAY_CHANNELS/8)
#define	REPLAY_NBSOUNDBUFFER 2

typedef void (CALLBACK *USER_CALLBACK)(void *, int *, int);

int m_bServerRunning = FALSE;
int m_bufferSize;
int m_currentBuffer;
HWAVEOUT m_hWaveOut;
WAVEHDR m_waveHeader[REPLAY_NBSOUNDBUFFER];
int *m_pSoundBuffer[REPLAY_NBSOUNDBUFFER];
USER_CALLBACK m_pUserCallback;

// MMSYSTEM STUFF 
void fillNextBuffer()
{
	// strictly needed to avoid deadlocks!
	// MSDN says never ever use wave*** or system functions inside a callback but who cares...
	// in fact only waveOutReset can cause a deadlock when a callback calls other wave*** functions

	// Call the user function to fill the buffer with anything you want! :-)
	if(m_pUserCallback != 0)   
	{		
		// check if the buffer is already prepared (should not!)
		if((m_waveHeader[m_currentBuffer].dwFlags & WHDR_PREPARED) != 0) 
			waveOutUnprepareHeader(m_hWaveOut, &m_waveHeader[m_currentBuffer], sizeof(WAVEHDR));	
	
		(*m_pUserCallback)(0, m_pSoundBuffer[m_currentBuffer], m_bufferSize);
	
		// Prepare the buffer to be sent to the WaveOut API
		m_waveHeader[m_currentBuffer].lpData = (char *)m_pSoundBuffer[m_currentBuffer];
		m_waveHeader[m_currentBuffer].dwBufferLength = m_bufferSize;
		waveOutPrepareHeader(m_hWaveOut, &m_waveHeader[m_currentBuffer], sizeof(WAVEHDR));
	
		// Send the buffer the the WaveOut queue
		waveOutWrite(m_hWaveOut, &m_waveHeader[m_currentBuffer], sizeof(WAVEHDR));
	
		m_currentBuffer += 1;
		if(m_currentBuffer >= REPLAY_NBSOUNDBUFFER) m_currentBuffer = 0;
	}	
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if(uMsg == WOM_DONE) fillNextBuffer();	
}


int WINMM_open(USER_CALLBACK pUserCallback, long totalBufferedSoundLen)
{

	WAVEFORMATEX wfx;
	MMRESULT errCode;

	m_pUserCallback = NULL;
	m_bServerRunning = FALSE;
	m_currentBuffer = 0;
	
	m_pUserCallback = pUserCallback;
	m_bufferSize = totalBufferedSoundLen*(REPLAY_CHANNELS*(REPLAY_DEPTH/8)); // *4 if stereo *2 if mono	
			
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = REPLAY_CHANNELS;
	wfx.nSamplesPerSec = REPLAY_RATE;
	wfx.nAvgBytesPerSec = REPLAY_RATE * REPLAY_SAMPLELEN ;
	wfx.nBlockAlign = REPLAY_SAMPLELEN;
	wfx.wBitsPerSample = REPLAY_DEPTH;
	wfx.cbSize = 0;
	
	errCode = waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &wfx, (DWORD)waveOutProc, 0, (DWORD)CALLBACK_FUNCTION);
	if(errCode != MMSYSERR_NOERROR) {m_bServerRunning = FALSE; return FALSE;}
	
	// Alloc the sample buffers.
	for(int i = 0; i <= REPLAY_NBSOUNDBUFFER-1; i++)
	{
		m_pSoundBuffer[i] = (int*)malloc(m_bufferSize);
		memset(&m_waveHeader[i], 0, sizeof(WAVEHDR));
		memset(m_pSoundBuffer[i], 0, m_bufferSize);
	}

	// Fill all the sound buffers
	m_currentBuffer = 0;
	for(int i = 0; i <= REPLAY_NBSOUNDBUFFER-1; i++)
	{
		fillNextBuffer();
	}

	m_bServerRunning = TRUE;

	return TRUE;
}


void WINMM_close()
{
	if(m_bServerRunning == TRUE)
	{
		
		// this makes a callback stop updating the buffers
		m_pUserCallback = NULL;

		// just to be sure buffers done playing
		Sleep(20);

		// it may hang here !!
		//if(waveOutReset(m_hWaveOut) != 0) printf("waveOutReset error!\n"); 
		for(int i = 0; i <= REPLAY_NBSOUNDBUFFER-1; i++)
		{
			if((m_waveHeader[m_currentBuffer].dwFlags & WHDR_PREPARED) != 0)
				waveOutUnprepareHeader(m_hWaveOut,&m_waveHeader[i], sizeof(WAVEHDR));
				
			free(m_pSoundBuffer[i]);
		}
		
		//if(waveOutClose(m_hWaveOut) != 0) printf("waveOutClose error!\n");
		m_bServerRunning = FALSE;
	}
}


void WINMM_reset()
{
	// make sure sound server is running 
	if(m_bServerRunning == TRUE)
	{
		// Clear the sample buffers to make silence
		for(int i = 0; i <= REPLAY_NBSOUNDBUFFER-1; i++)
			memset(m_pSoundBuffer[i], 0, m_bufferSize);
		
		//m_currentBuffer = 0
		
		//for(int i = 0; i <= REPLAY_NBSOUNDBUFFER-1; i++) 
		//	fillNextBuffer();
	}
	
}


void rad_adlib_write(unsigned char adl_reg,unsigned char adl_data)
{
	YM3812Write(ym, 0, adl_reg);
	YM3812Write(ym, 1, adl_data);
}

void rad_adlib_reset()
{
	WINMM_reset();
	YM3812ResetChip(ym);	
}

void CALLBACK playcallback(void *userdata, unsigned char *stream, int length)
{
	static int c, cnt = 0;
	
	for(c = 0; c <= BUFFSMPL - 1; c++)
	{
		if(cnt >= framesmpl)
		{
			cnt = 0;
			rad_update_frame();
		}
		YM3812UpdateOne(ym, &buf[c], 1);
		cnt += 1;
	}
	memcpy(stream, &buf[0], length);
}

int LM_SND_Init()
{
	ym = YM3812Init(1, OPL2_INTERNAL_FREQ, FREQHZ);
	YM3812ResetChip(ym);
 
	if(WINMM_open((USER_CALLBACK)playcallback, BUFFSMPL) == FALSE)
	{
		return FALSE;
	}
		
	return TRUE;
}

int LM_SND_Deinit()
{	
	rad_stop_music();
	WINMM_close();
	YM3812Shutdown();
	return TRUE;
}

void rad_set_timer(int value)
{
	if(value == 0) value = 1;
	framesmpl = FREQHZ / value;
}










