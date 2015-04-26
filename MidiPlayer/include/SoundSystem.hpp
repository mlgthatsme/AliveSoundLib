/////////////////////////////////////////////////////////////////////////////
//
// File: SoundSystem.h
// Purpose: Sound system for the Alive Engine Remake
// Website: https://github.com/paulsapps/alive/
//
// Author: Michael Grima
// Author Contact: mlgthatsme@hotmail.com.au
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <iterator>
#include <SDL.h>
#include <algorithm>
#include <mutex>  
#include "vab.hpp"

void AliveInitAudio();
void AliveAudioSDLCallback(void *udata, Uint8 *stream, int len);

const int AliveAudioSampleRate = 44100;

class AliveAudioTone;

static class AliveAudioHelper
{
public:

	template<class T>
	static T Lerp(T from, T to, float t)
	{
		return from + ((to - from)*t);
	}

	static float SampleSint16ToFloat(Sint16 v)
	{
		return (v / 32767.0f);
	}
};

class AliveAudioProgram
{
public:
	std::vector<AliveAudioTone *> m_Tones;
};

class AliveAudioSample
{
public:
	Uint16 * m_SampleBuffer;
	unsigned int i_SampleSize;
	unsigned int i_SampleRate;

	float GetSample(float sampleOffset);
};

class AliveAudioTone
{
public:
	// volume 0-1
	float Volume;

	// panning -1 - 1
	float Pan;

	// Root Key
	unsigned char Center;
	unsigned char Shift;

	// Key range
	unsigned char Min;
	unsigned char Max;

	AliveAudioSample * m_Sample;
};

class AliveAudioSoundbank
{
public:
	AliveAudioSoundbank(std::string fileName);

	std::vector<AliveAudioSample *> m_Samples;
	std::vector<AliveAudioProgram *> m_Programs;
};

class AliveAudioVoice
{
public:
	AliveAudioTone * m_Tone;
	int i_Program;
	int Note = 0;
	bool Dead = false;
	float SampleOffset = 0;
	bool Loop = false;
	bool NoteOn = true;
	float Velocity = 1.0f;
	int m_TrackID = 0; // This is used to distinguish between sounds fx and music
	float m_TrackDelay = 0; // Used by the sequencer for perfect timing

	float AttackSpeed = 1;
	float ReleaseSpeed = 0;

	float ActiveAttackVolume = 0;
	float ActiveReleaseVolume = 1;

	float GetSample()
	{
		if (Dead)	// Dont return anything if dead. This voice should now be removed.
			return 0;

		if (!NoteOn)
		{
			ActiveReleaseVolume = AliveAudioHelper::Lerp<float>(ActiveReleaseVolume,0,ReleaseSpeed);
			if (ActiveReleaseVolume <= 0.05f) // Release is done. So the voice is done.
			{
				Dead = true;
				ActiveReleaseVolume = 0;
			}
		}
		else
		{
			ActiveAttackVolume = AliveAudioHelper::Lerp<float>(ActiveAttackVolume, 1.0f, AttackSpeed);
			if (ActiveAttackVolume > 1)
				ActiveAttackVolume = 1;
		}

		if (SampleOffset >= m_Tone->m_Sample->i_SampleSize)
		{
			if (Loop)
				SampleOffset = 0;
			else
			{
				Dead = true;
				return 0; // Voice is dead now, so don't return anything.
			}
		}
		float sampleRate = 0;
		float sampleFrameRate = pow(1.059463f, Note - m_Tone->Center) * (44100.0f / AliveAudioSampleRate);
		SampleOffset += (sampleFrameRate);
		return m_Tone->m_Sample->GetSample(SampleOffset) * ActiveAttackVolume * ActiveReleaseVolume * Velocity;
	}
};

static class AliveAudio
{
public:
	static std::vector<unsigned char> m_SoundsDat;
	static void PlayOneShot(int program, int note, float volume)
	{
		voiceListMutex.lock();
		for (auto tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
		{
			if (note >= tone->Min && note <= tone->Max)
			{
				AliveAudioVoice * voice = new AliveAudioVoice();
				voice->Note = note;
				voice->Velocity = volume;
				voice->m_Tone = tone;
				printf("Sample Rate %i\n", voice->m_Tone->m_Sample->i_SampleRate);
				m_Voices.push_back(voice);
			}
		}
		voiceListMutex.unlock();
	}

	static void NoteOn(int program, int note, char velocity, int trackID = 0, float trackDelay = 0)
	{
		if (!voiceListLocked)
			voiceListMutex.lock();
		for (auto tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
		{
			if (note >= tone->Min && note <= tone->Max)
			{
				AliveAudioVoice * voice = new AliveAudioVoice();
				voice->Note = note;
				voice->m_Tone = tone;
				voice->i_Program = program;
				voice->Velocity = velocity / 127.0f;
				voice->m_TrackID = trackID;
				voice->m_TrackDelay = trackDelay;
				m_Voices.push_back(voice);
			}
		}
		if (!voiceListLocked)
			voiceListMutex.unlock();
	}

	static void NoteOff(int program, int note, int trackID = 0)
	{
		if (!voiceListLocked)
			voiceListMutex.lock();
		for (auto voice : m_Voices)
		{
			if (voice->Note == note && voice->i_Program == program && voice->m_TrackID == trackID)
			{
				voice->NoteOn = false;
			}
		}
		if (!voiceListLocked)
			voiceListMutex.unlock();
	}

	static void DebugPlayFirstToneSample(int program, int tone)
	{
		PlayOneShot(program, (m_CurrentSoundbank->m_Programs[program]->m_Tones[tone]->Min + m_CurrentSoundbank->m_Programs[program]->m_Tones[tone]->Max) / 2,1);
	}

	static void LockNotes()
	{
		if (!voiceListLocked)
		{
			voiceListLocked = true;
			voiceListMutex.lock();
		}
		else
			throw "Voice list locked. Can't lock again.";
	}

	static void UnlockNotes()
	{
		if (voiceListLocked)
		{
			voiceListLocked = false;
			voiceListMutex.unlock();
		}
		else
			throw "Voice list unlocked. Can't unlock again.";
	}

	static void ClearAllTrackVoices(int trackID, bool forceKill = false)
	{
		LockNotes();

		std::vector<AliveAudioVoice *> deadVoices;

		for (auto voice : AliveAudio::m_Voices)
		{
			if (forceKill)
			{
				if (voice->m_TrackID == trackID) // Kill the voices no matter what. Cuts of any sounds = Ugly sound
				{
					deadVoices.push_back(voice);
				}
			}
			else
			{
				if (voice->m_TrackID == trackID && voice->SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
				{
					deadVoices.push_back(voice);
				}
			}
		}

		for (auto obj : deadVoices)
		{
			AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
		}

		UnlockNotes();
	}

	static AliveAudioSoundbank* m_CurrentSoundbank;
	static std::mutex voiceListMutex;
	static std::vector<AliveAudioVoice *> m_Voices;
	static bool Interpolation;
	static bool voiceListLocked;
};