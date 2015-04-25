/////////////////////////////////////////////////////////////////////////////
//
// File: SoundSystem.cpp
// Purpose: Sound system for the Alive Engine Remake
// Website: https://github.com/paulsapps/alive/
//
// Author: Michael Grima
// Author Contact: mlgthatsme@hotmail.com.au
//
/////////////////////////////////////////////////////////////////////////////

#include "SoundSystem.hpp"
#include <algorithm>

std::vector<unsigned char> AliveAudio::m_SoundsDat;
AliveAudioSoundbank * AliveAudio::m_CurrentSoundbank = nullptr;

std::vector<AliveAudioVoice *> AliveAudio::m_Voices;
std::mutex AliveAudio::voiceListMutex;
bool AliveAudio::Interpolation = true;

float AliveAudioSample::GetSample(float sampleOffset)
{
	if (!AliveAudio::Interpolation)
		return AliveAudioHelper::SampleSint16ToFloat(m_SampleBuffer[(int)sampleOffset]); // No interpolation. Faster but sounds jaggy.

	int roundedOffset = (int)floor(sampleOffset);
	return AliveAudioHelper::SampleSint16ToFloat(AliveAudioHelper::Lerp<Sint16>(m_SampleBuffer[roundedOffset], m_SampleBuffer[roundedOffset + 1], sampleOffset - roundedOffset));
}

void AliveInitAudio()
{
	// UGLY FIX ALL OF THIS
	// |
	// V
	SDL_AudioSpec waveSpec;
	waveSpec.callback = AliveAudioSDLCallback;
	waveSpec.userdata = nullptr;
	waveSpec.channels = 2;
	waveSpec.freq = AliveAudioSampleRate;
	waveSpec.samples = 1024;
	waveSpec.format = AUDIO_F32;

	/* Open the audio device */
	if (SDL_OpenAudio(&waveSpec, NULL) < 0){
		fprintf(stderr, "Failed to initialize audio: %s\n", SDL_GetError());
		exit(-1);
	}
	
	std::ifstream soundDatFile;
	soundDatFile.open("sounds.dat", std::ios::binary);
	if (!soundDatFile.is_open())
	{
		abort();
	}
	soundDatFile.seekg(0, std::ios::end);
	std::streamsize fileSize = soundDatFile.tellg();
	soundDatFile.seekg(0, std::ios::beg);
	AliveAudio::m_SoundsDat = std::vector<unsigned char>(fileSize + 1); // Plus one, just in case interpolating tries to go that one byte further!

	if (soundDatFile.read((char*)AliveAudio::m_SoundsDat.data(), fileSize))
	{
		// Tada!
	}

	AliveAudioSoundbank * testSoundbank = new AliveAudioSoundbank("MINES");
	AliveAudio::m_CurrentSoundbank = testSoundbank;

	SDL_PauseAudio(0);
}

float limiterValue = 0;

void CleanVoices()
{
	AliveAudio::voiceListMutex.lock();
	std::vector<AliveAudioVoice *> deadVoices;
	
	for (auto voice : AliveAudio::m_Voices)
	{
		if (voice->Dead)
		{
			deadVoices.push_back(voice);
		}
	}

	for (auto obj : deadVoices)
	{
		AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
	}
	AliveAudio::voiceListMutex.unlock();
}

void AliveAudioSDLCallback(void *udata, Uint8 *stream, int len)
{
	static float tick = 0;
	static int note = 0;

	AliveAudioSoundbank * currentSoundbank = AliveAudio::m_CurrentSoundbank;

	float * AudioStream = (float*)stream;
	int StreamLength = len / sizeof(float);

	//for (int i = 0; i < f32Length; i++)
	//{
	//	float sampleRateFix = pow(1.059463f, note - 2) * (44100.0f / AliveAudioSampleRate);

	//	f32Stream[i] = currentSoundbank->m_Programs[23]->m_Tones[1]->m_Sample->GetSample(tick);
	//	tick += (0.1f * sampleRateFix);
	//	if (tick >= currentSoundbank->m_Programs[23]->m_Tones[1]->m_Sample->i_SampleSize)
	//	{
	//		tick = 0;
	//		//note++;
	//	}
	//}

	memset(stream, 0, len);

	AliveAudio::voiceListMutex.lock();
	for (auto voice : AliveAudio::m_Voices)
	{
		for (int i = 0; i < StreamLength; i+=2)
		{
			float s = voice->GetSample();

			//voice->m_Tone->Pan

			float centerPan = voice->m_Tone->Pan;
			float leftPan = 1;
			float rightPan = 1;

			// Panning is screwed. TODO: FIX THIS

			if (centerPan > 0)
			{
				//leftPan = 1.0f - abs(centerPan);
			}
			if (centerPan < 0)
			{
				//rightPan = 1.0f - abs(centerPan);
			}

			float leftSample = s * leftPan;
			float rightSample = s * rightPan;

			SDL_MixAudioFormat((Uint8 *)(AudioStream + i), (const Uint8*)&leftSample, AUDIO_F32, sizeof(float), 20); // Left Channel
			SDL_MixAudioFormat((Uint8 *)(AudioStream + i + 1), (const Uint8*)&rightSample, AUDIO_F32, sizeof(float), 20); // Right Channel
		}
	}
	
	AliveAudio::voiceListMutex.unlock();
	
	CleanVoices();
}

AliveAudioSoundbank::AliveAudioSoundbank(std::string fileName)
{
	Vab mVab;
	mVab.LoadVhFile(fileName + ".VH");
	mVab.LoadVbFile(fileName + ".VB");

	for (auto offset : mVab.iOffs)
	{
		AliveAudioSample * sample = new  AliveAudioSample();
		sample->i_SampleSize = offset->iLengthOrDuration / sizeof(Uint16);
		sample->m_SampleBuffer = (Uint16*)(AliveAudio::m_SoundsDat.data() + offset->iFileOffset);
		sample->i_SampleRate = offset->iUnusedByEngine;
		m_Samples.push_back(sample);
	}

	for (int i = 0; i < 128; i++)
	{
		AliveAudioProgram * program = new AliveAudioProgram();
		for (int t = 0; t < mVab.iProgs[i]->iNumTones; t++)
		{
			AliveAudioTone * tone = new AliveAudioTone();
			tone->Volume = mVab.iProgs[i]->iTones[t]->iVol / 127.0f;
			tone->Center = mVab.iProgs[i]->iTones[t]->iCenter;
			tone->Shift = mVab.iProgs[i]->iTones[t]->iShift;
			tone->Pan = (mVab.iProgs[i]->iTones[t]->iPan / 64.0f) - 1.0f;
			tone->Min = mVab.iProgs[i]->iTones[t]->iMin;
			tone->Max = mVab.iProgs[i]->iTones[t]->iMax;
			tone->Pan = mVab.iProgs[i]->iTones[t]->iPan;
			tone->m_Sample = m_Samples[mVab.iProgs[i]->iTones[t]->iVag - 1];
			program->m_Tones.push_back(tone);
		}

		m_Programs.push_back(program);
	}
}