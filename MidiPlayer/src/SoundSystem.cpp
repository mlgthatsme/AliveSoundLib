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

std::vector<unsigned char> AliveAudio::m_SoundsDat;
AliveAudioSoundbank * AliveAudio::m_CurrentSoundbank = nullptr;
jsonxx::Object AliveAudio::m_MusicJson;

std::vector<AliveAudioVoice *> AliveAudio::m_Voices;
std::mutex AliveAudio::voiceListMutex;
std::vector<std::vector<Uint8>> AliveAudio::m_LoadedSeqData;
bool AliveAudio::Interpolation = true;
bool AliveAudio::voiceListLocked = false;
long long AliveAudio::currentSampleIndex = 0;

float AliveAudioSample::GetSample(float sampleOffset)
{
	if (!AliveAudio::Interpolation)
		return AliveAudioHelper::SampleSint16ToFloat(m_SampleBuffer[(int)sampleOffset]); // No interpolation. Faster but sounds jaggy.

	int roundedOffset = (int)floor(sampleOffset);
	return AliveAudioHelper::SampleSint16ToFloat(AliveAudioHelper::Lerp<Sint16>(m_SampleBuffer[roundedOffset], m_SampleBuffer[roundedOffset + 1], sampleOffset - roundedOffset));
}

void AliveInitAudio()
{
	SDL_Init(SDL_INIT_AUDIO);
	// UGLY FIX ALL OF THIS
	// |
	// V
	SDL_AudioSpec waveSpec;
	waveSpec.callback = AliveAudioSDLCallback;
	waveSpec.userdata = nullptr;
	waveSpec.channels = 2;
	waveSpec.freq = AliveAudioSampleRate;
	waveSpec.samples = 512;
	waveSpec.format = AUDIO_F32;

	/* Open the audio device */
	if (SDL_OpenAudio(&waveSpec, NULL) < 0){
		fprintf(stderr, "Failed to initialize audio: %s\n", SDL_GetError());
		exit(-1);
	}

	std::string musicJson = mgFileManager::ReadFileToString("data/json/music.json");
	jsonxx::Object obj;
	obj.parse(musicJson);
	AliveAudio::m_MusicJson = obj;
	
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

	SDL_PauseAudio(0);
}

void CleanVoices()
{
	AliveAudio::voiceListMutex.lock();
	std::vector<AliveAudioVoice *> deadVoices;
	
	for (auto voice : AliveAudio::m_Voices)
	{
		if (voice->b_Dead)
		{
			deadVoices.push_back(voice);
		}
	}

	for (auto obj : deadVoices)
	{
		delete obj;

		AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
	}
	AliveAudio::voiceListMutex.unlock();
}

void AliveRenderAudio(float * AudioStream, int StreamLength)
{
	static float tick = 0;
	static int note = 0;

	AliveAudioSoundbank * currentSoundbank = AliveAudio::m_CurrentSoundbank;

	AliveAudio::voiceListMutex.lock();
	int voiceCount = AliveAudio::m_Voices.size();
	AliveAudioVoice ** rawPointer = AliveAudio::m_Voices.data(); // Real nice speed boost here.

	for (int i = 0; i < StreamLength; i += 2)
	{
		for (int v = 0; v < voiceCount; v++)
		{
			AliveAudioVoice * voice = rawPointer[v]; // Raw pointer skips all that vector bottleneck crap

			voice->f_TrackDelay--;

			if (voice->m_UsesNoteOffDelay)
				voice->f_NoteOffDelay--;

			if (voice->m_UsesNoteOffDelay && voice->f_NoteOffDelay <= 0 && voice->b_NoteOn == true)
			{
				voice->b_NoteOn = false;
				//printf("off");
			}

			if (voice->b_Dead || voice->f_TrackDelay > 0)
				continue;

			float centerPan = voice->m_Tone->f_Pan;
			float leftPan = 1.0f;
			float rightPan = 1.0f;

			if (centerPan > 0)
			{
				leftPan = 1.0f - abs(centerPan);
			}
			if (centerPan < 0)
			{
				rightPan = 1.0f - abs(centerPan);
			}

			float s = voice->GetSample();

			float leftSample = s * leftPan;
			float rightSample = s * rightPan;

			SDL_MixAudioFormat((Uint8 *)(AudioStream + i), (const Uint8*)&leftSample, AUDIO_F32, sizeof(float), 20); // Left Channel
			SDL_MixAudioFormat((Uint8 *)(AudioStream + i + 1), (const Uint8*)&rightSample, AUDIO_F32, sizeof(float), 20); // Right Channel
		}

		AliveAudio::currentSampleIndex++;
	}
	AliveAudio::voiceListMutex.unlock();


	CleanVoices();
}

void AliveAudioSDLCallback(void *udata, Uint8 *stream, int len)
{
	memset(stream, 0, len);
	AliveRenderAudio((float *)stream, len / sizeof(float));
}

#define BINARY_GETMASK(index, size) (((1 << size) - 1) << index)
#define BINARY_READFROM(data, index, size) ((data & BINARY_GETMASK(index, size)) >> index)
#define BINARY_WRITETO(data, index, size, value) (data = (data & (~BINARY_GETMASK(index, size))) | (value << index))

AliveAudioSoundbank::~AliveAudioSoundbank()
{
	for (auto program : m_Programs)
	{
		for (auto tone : program->m_Tones)
		{
			delete tone;
		}

		delete program;
	}
}

AliveAudioSoundbank::AliveAudioSoundbank(std::string fileName)
{
	std::ifstream vbStream;
	std::ifstream vhStream;
	vbStream.open((fileName + ".VB").c_str(), std::ios::binary);
	vhStream.open((fileName + ".VH").c_str(), std::ios::binary);

	Vab mVab;
	mVab.ReadVh(vhStream);
	mVab.ReadVb(vbStream);

	InitFromVab(mVab);

	vbStream.close();
	vhStream.close();
}

AliveAudioSoundbank::AliveAudioSoundbank(Oddlib::LvlArchive& archive, std::string vabID)
{
	Oddlib::LvlArchive::File * vhFile = archive.FileByName(vabID + ".VH");
	Oddlib::LvlArchive::File * vbFile = archive.FileByName(vabID + ".VB");

	std::vector<Uint8> vhData = vhFile->ChunkById(0)->ReadData();
	std::vector<Uint8> vbData = vbFile->ChunkById(0)->ReadData();

	std::stringstream vhStream;
	vhStream.write((const char *)vhData.data(), vhData.size());
	vhStream.seekg(0, std::ios_base::beg);

	std::stringstream vbStream;
	vbStream.write((const char *)vbData.data(), vbData.size());
	vbStream.seekg(0, std::ios_base::beg);

	Vab vab = Vab();
	vab.ReadVh(vhStream);
	vab.ReadVb(vbStream);

	InitFromVab(vab);
}

AliveAudioSoundbank::AliveAudioSoundbank(std::string lvlPath, std::string vabID)
{
	Oddlib::LvlArchive archive = Oddlib::LvlArchive(lvlPath);
	Oddlib::LvlArchive::File * vhFile = archive.FileByName(vabID + ".VH");
	Oddlib::LvlArchive::File * vbFile = archive.FileByName(vabID + ".VB");

	std::vector<Uint8> vhData = vhFile->ChunkById(0)->ReadData();
	std::vector<Uint8> vbData = vbFile->ChunkById(0)->ReadData();

	std::stringstream vhStream;
	vhStream.write((const char *)vhData.data(), vhData.size());
	vhStream.seekg(0, std::ios_base::beg);

	std::stringstream vbStream;
	vbStream.write((const char *)vbData.data(), vbData.size());
	vbStream.seekg(0, std::ios_base::beg);

	Vab vab = Vab();
	vab.ReadVh(vhStream);
	vab.ReadVb(vbStream);

	InitFromVab(vab);
}

void AliveAudioSoundbank::InitFromVab(Vab& mVab)
{
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

			//printf("Program:%i\tTone: %i\tReserved:%i\n",i, t, mVab.iProgs[i]->iTones[t]->);

			tone->f_Volume = mVab.iProgs[i]->iTones[t]->iVol / 127.0f;
			tone->c_Center = mVab.iProgs[i]->iTones[t]->iCenter;
			tone->c_Shift = mVab.iProgs[i]->iTones[t]->iShift;
			tone->f_Pan = (mVab.iProgs[i]->iTones[t]->iPan / 64.0f) - 1.0f;
			tone->Min = mVab.iProgs[i]->iTones[t]->iMin;
			tone->Max = mVab.iProgs[i]->iTones[t]->iMax;
			tone->Pitch = mVab.iProgs[i]->iTones[t]->iShift / 100.0f;
			tone->m_Sample = m_Samples[mVab.iProgs[i]->iTones[t]->iVag - 1];
			program->m_Tones.push_back(tone);

			unsigned short ADSR1 = mVab.iProgs[i]->iTones[t]->iAdsr1;
			unsigned short ADSR2 = mVab.iProgs[i]->iTones[t]->iAdsr2;


			unsigned short attackRate = BINARY_READFROM(ADSR1, 1, 7);
			unsigned short sustainRate = ((ADSR2 & 0x2000) >> 13);
			/*printf("Attack Rate Expon: %i\n", (ADSR1 & 0x8000) >> 15);
			printf("Attack Rate: %i\n", attackRate);
			printf("Decay Rate: %i\n", 15 - ((ADSR1 & 0xF0) >> 4));
			printf("Sustain Level: %i\n", (ADSR1 & 0x0F));

			printf("Sustain Rate Expon: %i\n", (ADSR2 & 0x8000) >> 15);
			printf("Sustain Rate Sign: %i\n", (ADSR2 & 0x4000) >> 14);
			printf("Sustain Rate: %i\n", sustainRate);

			printf("Release Rate: %i\n", 31 - (ADSR2 & 0x001F));
			printf("Release Rate Expon: %i\n", (ADSR2 & 0x20) >> 5);*/

			tone->AttackSpeed = attackRate / 127.0f;
			tone->DecaySpeed = (15 - ((ADSR1 & 0xF0) >> 4)) / 15.0f;
			tone->SustainSpeed = sustainRate / 127.0f;
			tone->ReleaseSpeed = (31 - (ADSR2 & 0x001F)) / 31.0f;
			tone->ReleaseExponential = ((ADSR2 & 0x20) >> 5);

			if ((i == 14 && t == 11) || (i == 27 && t == 0)) // Do loop hacks here
			{
				tone->Loop = true;
			}
		}

		m_Programs.push_back(program);
	}
}