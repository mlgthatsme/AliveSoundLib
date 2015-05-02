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
#include "json\jsonxx.h"
#include "FileManager.h"
#include "oddlib\lvlarchive.hpp"
#include "ADSR.h"
//#include "reverb.h"

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

	static float RandFloat(float a, float b)
	{
		return ((b - a)*((float)rand() / RAND_MAX)) + a;
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

	float GetSample(float sampleOffset);
};

class AliveAudioTone
{
public:
	// volume 0-1
	float f_Volume;

	// panning -1 - 1
	float f_Pan;

	// Root Key
	unsigned char c_Center;
	unsigned char c_Shift;

	// Key range
	unsigned char Min;
	unsigned char Max;

	float Pitch;

	double AttackTime;
	double ReleaseTime;
	bool ReleaseExponential = false;
	double DecayTime;
	double SustainTime;

	bool Loop = false;

	AliveAudioSample * m_Sample;
};

class AliveAudioSoundbank
{
public:
	~AliveAudioSoundbank();
	AliveAudioSoundbank(std::string fileName);
	AliveAudioSoundbank(std::string lvlPath, std::string vabName);
	AliveAudioSoundbank(Oddlib::LvlArchive& lvlArchive, std::string vabName);

	void InitFromVab(Vab& vab);
	std::vector<AliveAudioSample *> m_Samples;
	std::vector<AliveAudioProgram *> m_Programs;
};

class AliveAudioVoice
{
public:
	AliveAudioTone * m_Tone;
	int		i_Program;
	int		i_Note = 0;
	bool	b_Dead = false;
	double	f_SampleOffset = 0;
	bool	b_NoteOn = true;
	double	f_Velocity = 1.0f;
	double	f_Pitch = 0.0f;

	int		i_TrackID = 0; // This is used to distinguish between sounds fx and music
	double	f_TrackDelay = 0; // Used by the sequencer for perfect timing
	bool	m_UsesNoteOffDelay = false;
	double	f_NoteOffDelay = 0;

	// Active ADSR Levels

	double ActiveAttackLevel = 0;
	double ActiveReleaseLevel = 1;
	double ActiveDecayLevel = 1;
	double ActiveSustainLevel = 1;

	float GetSample()
	{
		if (b_Dead)	// Dont return anything if dead. This voice should now be removed.
			return 0;

		// ActiveDecayLevel -= ((1.0 / AliveAudioSampleRate) / m_Tone->DecayTime);

		if (ActiveDecayLevel < 0)
			ActiveDecayLevel = 0;

		if (!b_NoteOn)
		{
			ActiveReleaseLevel -= ((1.0 / AliveAudioSampleRate) / m_Tone->ReleaseTime);

			ActiveReleaseLevel -= ((1.0 / AliveAudioSampleRate) / m_Tone->DecayTime); // Hacks?!?! --------------------

			if (ActiveReleaseLevel <= 0) // Release is done. So the voice is done.
			{
				b_Dead = true;
				ActiveReleaseLevel = 0;
			}
		}
		else
		{
			ActiveAttackLevel += ((1.0 / AliveAudioSampleRate) / m_Tone->AttackTime);

			if (ActiveAttackLevel > 1)
				ActiveAttackLevel = 1;
		}

		if (f_SampleOffset >= m_Tone->m_Sample->i_SampleSize)
		{
			if (m_Tone->Loop)
				f_SampleOffset = 0;
			else
			{
				b_Dead = true;
				return 0; // Voice is dead now, so don't return anything.
			}
		}

		double sampleFrameRate = pow(1.059463, i_Note - m_Tone->c_Center + m_Tone->Pitch + f_Pitch) * (44100.0 / AliveAudioSampleRate);
		f_SampleOffset += (sampleFrameRate);

		return m_Tone->m_Sample->GetSample(f_SampleOffset) * ActiveAttackLevel * ActiveDecayLevel * ((b_NoteOn) ? ActiveSustainLevel : ActiveReleaseLevel) * f_Velocity;
	}
};

static class AliveAudio
{
public:
	static std::vector<unsigned char> m_SoundsDat;
	static void PlayOneShot(int program, int note, float volume, float pitch = 0)
	{
		voiceListMutex.lock();
		for (auto tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
		{
			if (note >= tone->Min && note <= tone->Max)
			{
				AliveAudioVoice * voice = new AliveAudioVoice();
				voice->i_Note = note;
				voice->f_Velocity = volume;
				voice->m_Tone = tone;
				voice->f_Pitch = pitch;
				m_Voices.push_back(voice);
			}
		}
		voiceListMutex.unlock();
	}

	static void PlayOneShot(std::string soundID)
	{
		jsonxx::Array soundList = AliveAudio::m_Config.get<jsonxx::Array>("sounds");

		for (int i = 0; i < soundList.size(); i++)
		{
			jsonxx::Object sndObj = soundList.get<jsonxx::Object>(i);
			if (sndObj.get<jsonxx::String>("id") == soundID)
			{
				float randA = 0;
				float randB = 0;
				
				if (sndObj.has<jsonxx::Array>("pitchrand"))
				{
					randA = (float)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(0);
					randB = (float)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(1);
				}

				PlayOneShot((int)sndObj.get<jsonxx::Number>("prog"), (int)sndObj.get<jsonxx::Number>("note"), 1.0f, AliveAudioHelper::RandFloat(randA,randB));
			}
		}
	}

	static void NoteOn(int program, int note, char velocity, float pitch = 0, int trackID = 0, float trackDelay = 0)
	{
		if (!voiceListLocked)
			voiceListMutex.lock();
		for (auto tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
		{
			if (note >= tone->Min && note <= tone->Max)
			{
				AliveAudioVoice * voice = new AliveAudioVoice();
				voice->i_Note = note;
				voice->m_Tone = tone;
				voice->i_Program = program;
				voice->f_Velocity = velocity / 127.0f;
				voice->i_TrackID = trackID;
				voice->f_TrackDelay = trackDelay;
				m_Voices.push_back(voice);
			}
		}
		if (!voiceListLocked)
			voiceListMutex.unlock();
	}

	static void NoteOn(int program, int note, char velocity, int trackID = 0, float trackDelay = 0)
	{
		NoteOn(program, note, velocity, 0, trackID, trackDelay);
	}

	static void NoteOff(int program, int note, int trackID = 0)
	{
		if (!voiceListLocked)
			voiceListMutex.lock();
		for (auto voice : m_Voices)
		{
			if (voice->i_Note == note && voice->i_Program == program && voice->i_TrackID == trackID)
			{
				voice->b_NoteOn = false;
			}
		}
		if (!voiceListLocked)
			voiceListMutex.unlock();
	}

	static void NoteOffDelay(int program, int note, int trackID = 0, float trackDelay = 0)
	{
		if (!voiceListLocked)
			voiceListMutex.lock();
		for (auto voice : m_Voices)
		{
			if (voice->i_Note == note && voice->i_Program == program && voice->i_TrackID == trackID && voice->f_TrackDelay < trackDelay && voice->f_NoteOffDelay <= 0)
			{
				voice->m_UsesNoteOffDelay = true;
				voice->f_NoteOffDelay = trackDelay;
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
		//else
			//throw "Voice list locked. Can't lock again.";
	}

	static void UnlockNotes()
	{
		if (voiceListLocked)
		{
			voiceListLocked = false;
			voiceListMutex.unlock();
		}
		//else
			//throw "Voice list unlocked. Can't unlock again.";
	}

	static void ClearAllVoices(bool forceKill = true)
	{
		LockNotes();

		std::vector<AliveAudioVoice *> deadVoices;

		for (auto voice : AliveAudio::m_Voices)
		{
			if (forceKill)
			{
				deadVoices.push_back(voice);
			}
			else
			{
				voice->b_NoteOn = false; // Send a note off to all of the notes though.
				if (voice->f_SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
				{
					deadVoices.push_back(voice);
				}
			}
		}

		for (auto obj : deadVoices)
		{
			delete obj;

			AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
		}

		UnlockNotes();
	}

	static void ClearAllTrackVoices(int trackID, bool forceKill = false)
	{
		LockNotes();

		std::vector<AliveAudioVoice *> deadVoices;

		for (auto voice : AliveAudio::m_Voices)
		{
			if (forceKill)
			{
				if (voice->i_TrackID == trackID) // Kill the voices no matter what. Cuts of any sounds = Ugly sound
				{
					deadVoices.push_back(voice);
				}
			}
			else
			{
				voice->b_NoteOn = false; // Send a note off to all of the notes though.
				if (voice->i_TrackID == trackID && voice->f_SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
				{
					deadVoices.push_back(voice);
				}
			}
		}

		for (auto obj : deadVoices)
		{
			delete obj;

			AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
		}

		UnlockNotes();
	}

	static void LoadSoundbank(char * fileName)
	{
		ClearAllVoices(true);

		if (AliveAudio::m_CurrentSoundbank != nullptr)
			delete AliveAudio::m_CurrentSoundbank;

		AliveAudioSoundbank * soundBank = new AliveAudioSoundbank(fileName);
		AliveAudio::m_CurrentSoundbank = soundBank;
	}

	static void SetSoundbank(AliveAudioSoundbank * soundbank)
	{
		ClearAllVoices(true);

		if (AliveAudio::m_CurrentSoundbank != nullptr)
			delete AliveAudio::m_CurrentSoundbank;

		AliveAudio::m_CurrentSoundbank = soundbank;
	}

	static void LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile)
	{
		m_LoadedSeqData.clear();
		Oddlib::LvlArchive archive(lvlPath);
		SetSoundbank(new AliveAudioSoundbank(archive, vabID));
		for (int i = 0; i < archive.FileByName(seqFile)->GetChunkCount(); i++)
		{
			m_LoadedSeqData.push_back(archive.FileByName(seqFile)->ChunkByIndex(i)->ReadData());
		}
	}

	static AliveAudioSoundbank* m_CurrentSoundbank;
	//static std::vector<AliveAudioTrack*> m_CurrentTrackList;
	static std::vector<std::vector<Uint8>> m_LoadedSeqData;
	static std::mutex voiceListMutex;
	static std::vector<AliveAudioVoice *> m_Voices;
	static bool Interpolation;
	static bool voiceListLocked;
	static long long currentSampleIndex;

	static jsonxx::Object m_Config;
};