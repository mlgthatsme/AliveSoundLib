#pragma once

#include "SDL.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>

#include "vab.hpp"
#include "FileManager.h"
#include "oddlib\lvlarchive.hpp"
#include "json\jsonxx.h"

#include "Soundbank.h"
#include "Helper.h"
#include "Program.h"
#include "Tone.h"
#include "Voice.h"
#include "Sample.h"
#include "ADSR.h"

void AliveInitAudio();
void AliveAudioSDLCallback(void *udata, Uint8 *stream, int len);

const int AliveAudioSampleRate = 44100;

static class AliveAudio
{
public:
	static std::vector<unsigned char> m_SoundsDat;
	static void PlayOneShot(int program, int note, float volume, float pitch = 0);
	static void PlayOneShot(std::string soundID);

	static void NoteOn(int program, int note, char velocity, float pitch = 0, int trackID = 0, float trackDelay = 0);
	static void NoteOn(int program, int note, char velocity, int trackID = 0, float trackDelay = 0);

	static void NoteOff(int program, int note, int trackID = 0);
	static void NoteOffDelay(int program, int note, int trackID = 0, float trackDelay = 0);

	static void DebugPlayFirstToneSample(int program, int tone);

	static void LockNotes();
	static void UnlockNotes();

	static void ClearAllVoices(bool forceKill = true);
	static void ClearAllTrackVoices(int trackID, bool forceKill = false);

	static void LoadSoundbank(char * fileName);
	static void SetSoundbank(AliveAudioSoundbank * soundbank);

	static void LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile);

	static AliveAudioSoundbank* m_CurrentSoundbank;
	static std::vector<std::vector<Uint8>> m_LoadedSeqData;
	static std::mutex voiceListMutex;
	static std::vector<AliveAudioVoice *> m_Voices;
	static bool Interpolation;
	static bool voiceListLocked;
	static long long currentSampleIndex;
	static jsonxx::Object m_Config;
};