#pragma once

#include <SDL.h>
#include <string>
#include <thread>
#include <vector>
#include "oddlib\stream.hpp"
#include "SoundSystem.hpp"
#include "PreciseTimer.h"

struct SeqHeader
{
	Uint32 mMagic; // SEQp
	Uint32 mVersion; // Seems to always be 1
	Uint16 mResolutionOfQuaterNote;
	Uint8 mTempo[3];
	Uint16 mRhythm;

};

struct SeqInfo
{
	Uint32 iLastTime = 0;
	Sint32 iNumTimesToLoop = 0;
	Uint8 running_status = 0;
};

enum AliveAudioMidiMessageType
{
	ALIVE_MIDI_NOTE_ON = 1,
	ALIVE_MIDI_NOTE_OFF = 2,
	ALIVE_MIDI_PROGRAM_CHANGE = 3,
	ALIVE_MIDI_ENDTRACK = 4,
};

enum AliveAudioSequencerState
{
	ALIVE_SEQUENCER_STOPPED = 1,
	ALIVE_SEQUENCER_PAUSED = 2,
	ALIVE_SEQUENCER_PLAYING = 3,
	ALIVE_SEQUENCER_FINISHED = 4,
};

struct AliveAudioMidiMessage
{
	AliveAudioMidiMessage(AliveAudioMidiMessageType type, int timeOffset, int channel, int note, int velocity, int special = 0)
	{
		Type = type;
		Channel = channel;
		Note = note;
		Velocity = velocity;
		TimeOffset = timeOffset;
		Special = special;
	}
	AliveAudioMidiMessageType Type;
	int Channel;
	int Note;
	int Velocity;
	int TimeOffset;
	int Special = 0;
};

class SequencePlayer
{
public:
	SequencePlayer();
	~SequencePlayer();

	std::vector<AliveAudioMidiMessage> MessageList;
	int LoadSequence(std::string filePath);
	void PlaySequence();
	void PauseSequence();
	void StopSequence();
private:
	void threadWork();
	int playTick = 0;
	float tempoMilliseconds = 0;
	std::thread * sequenceThread;
	std::mutex midiListMutex;
	std::mutex stateMutex;
	int killThread = false;
	mgPreciseTimer timer;
	int trackID = 1;
	AliveAudioSequencerState sequencerState = ALIVE_SEQUENCER_STOPPED;
};

