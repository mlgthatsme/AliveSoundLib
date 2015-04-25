#include "SequencePlayer.h"
#include "oddlib\stream.hpp"
#include "SoundSystem.hpp"
#include "PreciseTimer.h"

SequencePlayer::SequencePlayer()
{
}


SequencePlayer::~SequencePlayer()
{
}

void snd_midi_skip_length(Oddlib::Stream& stream, int skip)
{
	stream.Seek(stream.Pos() + skip);
}

static Uint32 midi_read_var_len(Oddlib::Stream& stream)
{
	Uint32 ret = 0;
	Uint8 byte = 0;
	for (int i = 0; i < 4; ++i)
	{
		stream.ReadUInt8(byte);
		ret = (ret << 7) | (byte & 0x7f);
		if (!(byte & 0x80))
		{
			break;
		}
	}
	return ret;
}

void TestPlay(std::vector<AliveAudioMidiMessage> MessageList)
{
	int tick = 0;
	bool playing = true;
	int channels[16];
	memset(channels, 0, sizeof(channels));
	mgPreciseTimer timer;
	
	while (playing)
	{
		timer.Start();

		for (auto m : MessageList)
		{
			if (m.TimeOffset == tick)
			{
				switch (m.Type)
				{
				case ALIVE_MIDI_NOTE_ON:
					AliveAudio::NoteOn(channels[m.Channel], m.Note, m.Velocity);
					break;
				case ALIVE_MIDI_NOTE_OFF:
					AliveAudio::NoteOff(channels[m.Channel], m.Note);
					break;
				case ALIVE_MIDI_PROGRAM_CHANGE:
					channels[m.Channel] = m.Special;
					break;
				case ALIVE_MIDI_ENDTRACK:
					//playing = false;
					tick = 0;
					break;
				}
			}
		}

		float makeUpTime = timer.GetDuration();
		tick++;
		while (timer.GetDuration() < 0.00065f - makeUpTime); // This is just a random delay. Needs to be calculated from seq file
	}
}

int SequencePlayer::PlayMidiFile()
{
	std::ifstream soundDatFile;
	soundDatFile.open("TEST.SEQ", std::ios::binary);
	if (!soundDatFile.is_open())
	{
		abort();
	}
	soundDatFile.seekg(0, std::ios::end);
	std::streamsize fileSize = soundDatFile.tellg();
	soundDatFile.seekg(0, std::ios::beg);
	std::vector<unsigned char> seqData = std::vector<unsigned char>(fileSize + 20); // Plus one, just in case aliasing tries to go that one byte further!
	soundDatFile.read((char*)seqData.data(), fileSize);

	Oddlib::Stream stream(std::move(seqData));

	// Read the header
	SeqHeader header;
	stream.ReadUInt32(header.mMagic);
	stream.ReadUInt32(header.mVersion);
	stream.ReadUInt16(header.mResolutionOfQuaterNote);
	stream.ReadBytes(header.mTempo, sizeof(header.mTempo));
	stream.ReadUInt16(header.mRhythm);

	int channels[16];

	static unsigned int deltaTime = 0;

	const size_t midiDataStart = stream.Pos();

	// Context state
	SeqInfo gSeqInfo = {};

	std::vector<AliveAudioMidiMessage> MessageList;

	for (;;)
	{
		// Read event delta time
		Uint32 delta = midi_read_var_len(stream);
		deltaTime += delta;
		//std::cout << "Delta: " << delta << " over all " << deltaTime << std::endl;

		// Obtain the event/status byte
		Uint8 eventByte = 0;
		stream.ReadUInt8(eventByte);
		if (eventByte < 0x80)
		{
			// Use running status
			if (!gSeqInfo.running_status) // v1
			{
				return 0; // error if no running status?
			}
			eventByte = gSeqInfo.running_status;

			// Go back one byte as the status byte isn't a status
			stream.Seek(stream.Pos() - 1);
		}
		else
		{
			// Update running status
			gSeqInfo.running_status = eventByte;
		}

		if (eventByte == 0xff)
		{
			// Meta event
			Uint8 metaCommand = 0;
			stream.ReadUInt8(metaCommand);

			Uint8 metaCommandLength = 0;
			stream.ReadUInt8(metaCommandLength);

			switch (metaCommand)
			{
			case 0x2f:
			{
				//std::cout << "end of track" << std::endl;
				MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_ENDTRACK, deltaTime, 0, 0, 0));
				TestPlay(MessageList);
				return 0;
				int loopCount = gSeqInfo.iNumTimesToLoop;// v1 some hard coded data?? or just a local static?
				if (loopCount) // If zero then loop forever
				{
					--loopCount;

					//char buf[256];
					//sprintf(buf, "EOT: %d loops left\n", loopCount);
					// OutputDebugString(buf);

					gSeqInfo.iNumTimesToLoop = loopCount; //v1
					if (loopCount <= 0)
					{
						//getNext_q(aSeqIndex); // Done playing? Ptr not reset to start
						return 1;
					}
				}

				//OutputDebugString("EOT: Loop forever\n");
				// Must be a loop back to the start?
				stream.Seek(midiDataStart);

			}

			case 0x51:    // Tempo in microseconds per quarter note (24-bit value)
			{
				//std::cout << "Temp change" << std::endl;
				// TODO: Not sure if this is correct
				Uint8 tempoByte = 0;
				for (int i = 0; i < 3; i++)
				{
					stream.ReadUInt8(tempoByte);
				}
			}
			break;

			default:
			{
				//std::cout << "Unknown meta event " << Uint32(metaCommand) << std::endl;
				// Skip unknown events
				// TODO Might be wrong
				snd_midi_skip_length(stream, metaCommandLength);
			}

			}
		}
		else if (eventByte < 0x80)
		{
			// Error
			throw std::runtime_error("Unknown midi event");
		}
		else
		{
			const Uint8 channel = eventByte & 0xf;
			switch (eventByte >> 4)
			{
			case 0x9:
			{
				Uint8 note = 0;
				stream.ReadUInt8(note);

				Uint8 velocity = 0;
				stream.ReadUInt8(velocity);

				//std::cout << Uint32(channel) << " NOTE ON " << Uint32(note) << " vel " << Uint32(velocity) << std::endl;

				if (velocity == 0)
				{
					MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_NOTE_OFF, deltaTime, channel, note, velocity));
					//AliveAudio::NoteOff(channels[channel], note);
				}
				else
				{
					MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_NOTE_ON, deltaTime, channel, note, velocity));
					//AliveAudio::NoteOn(channels[channel], note, velocity);
				}
			}
			break;
			case 0x8:
			{
				Uint8 note = 0;
				stream.ReadUInt8(note);

				Uint8 velocity = 0;
				stream.ReadUInt8(velocity);

				//std::cout << Uint32(channel) << " NOTE OFF " << Uint32(note) << " vel " << Uint32(velocity) << std::endl;

				//AliveAudio::NoteOff(channels[channel], note);
				MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_NOTE_OFF, deltaTime, channel, note, velocity));
			}
			break;
			case 0xc:
			{
				Uint8 prog = 0;
				stream.ReadUInt8(prog);
				//std::cout << Uint32(channel) << " program change " << Uint32(prog) << std::endl;
				//channels[channel] = prog;
				MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_PROGRAM_CHANGE, deltaTime, channel, 0, 0, prog));
			}
			break;
			case 0xa:
			{
				Uint8 note = 0;
				Uint8 pressure = 0;

				stream.ReadUInt8(note);
				stream.ReadUInt8(pressure);
				std::cout << Uint32(channel) << " polyphonic key pressure (after touch)" << Uint32(note) << " " << Uint32(pressure) << std::endl;
			}
			break;
			case 0xb:
			{
				Uint8 controller = 0;
				Uint8 value = 0;
				stream.ReadUInt8(controller);
				stream.ReadUInt8(value);
				std::cout << Uint32(channel) << " controller change " << Uint32(controller) << " value " << Uint32(value);
			}
			break;
			case 0xd:
			{
				Uint8 value = 0;
				stream.ReadUInt8(value);
				std::cout << Uint32(channel) << " after touch " << Uint32(value) << std::endl;
			}
			break;
			case 0xe:
			{
				Uint16 bend = 0;
				stream.ReadUInt16(bend);
				std::cout << Uint32(channel) << " pitch bend " << Uint32(bend) << std::endl;
			}
			break;
			case 0xf:
			{
				const Uint32 length = midi_read_var_len(stream);
				snd_midi_skip_length(stream, length);
				std::cout << Uint32(channel) << " sysex len: " << length << std::endl;
			}
			break;
			default:
				throw std::runtime_error("Unknown MIDI command");
			}
			
		}
	}
}