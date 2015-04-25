#pragma once

#include <SDL.h>

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

	static int PlayMidiFile();
};

