#include "SequencePlayer.h"

SequencePlayer::SequencePlayer()
{
	sequenceThread = new std::thread(&SequencePlayer::threadWork, this);
}

SequencePlayer::~SequencePlayer()
{
	killThread = true;
	sequenceThread->join();
	delete sequenceThread;
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

void SequencePlayer::threadWork()
{
	int channels[16];
	memset(channels, 0, sizeof(channels));
	static int delayTotal = 0;
	while (!killThread)
	{
		stateMutex.lock();
		if (sequencerState == ALIVE_SEQUENCER_PLAYING)
		{
			for (int i = 0; i < MessageList.size();i++)
			{
				AliveAudio::LockNotes();
				AliveAudioMidiMessage m = MessageList[i];
				delayTotal += m.TimeOffset;
				switch (m.Type)
				{
				case ALIVE_MIDI_NOTE_ON:
					AliveAudio::NoteOn(channels[m.Channel], m.Note, m.Velocity, trackID, delayTotal * 30);
					break;
				case ALIVE_MIDI_NOTE_OFF:
					//AliveAudio::NoteOff(channels[m.Channel], m.Note); // Fix this. Make note off's have an offset in the voice timeline.
					break;
				case ALIVE_MIDI_PROGRAM_CHANGE:
					channels[m.Channel] = m.Special;
					break;
				case ALIVE_MIDI_ENDTRACK:
					sequencerState = ALIVE_SEQUENCER_FINISHED;
					delayTotal = 0;
					break;
				}
				AliveAudio::UnlockNotes();
			}
		}
		stateMutex.unlock();
	}
}

void SequencePlayer::StopSequence()
{
	AliveAudio::ClearAllTrackVoices(trackID);
	stateMutex.lock();
	sequencerState = ALIVE_SEQUENCER_STOPPED;
	playTick = 0;
	stateMutex.unlock();
}

void SequencePlayer::PlaySequence()
{
	stateMutex.lock();
	if (sequencerState == ALIVE_SEQUENCER_STOPPED || sequencerState == ALIVE_SEQUENCER_FINISHED)
	{
		playTick = 0;
	}

	sequencerState = ALIVE_SEQUENCER_PLAYING;
	stateMutex.unlock();
}

void SequencePlayer::PauseSequence()
{
	stateMutex.lock();
	sequencerState = ALIVE_SEQUENCER_PAUSED;
	stateMutex.unlock();
}

int SequencePlayer::LoadSequence(std::string filePath)
{
	StopSequence();
	MessageList.clear();
	std::ifstream soundDatFile;
	soundDatFile.open(filePath.c_str(), std::ios::binary);
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

	int tempoValue = 0;
	for (int i = 0; i < 3; i++)
	{
		tempoValue += header.mTempo[i] << (8 * i);
	}

	//tempoMilliseconds = (60000000.0f / tempoValue) / 1000.0f;

	tempoMilliseconds = tempoValue / (header.mResolutionOfQuaterNote * 50000.0f);

	int channels[16];

	unsigned int deltaTime = 0;

	const size_t midiDataStart = stream.Pos();

	// Context state
	SeqInfo gSeqInfo = {};

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
				MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_ENDTRACK, delta, 0, 0, 0));
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
				int t = 0;
				for (int i = 0; i < 3; i++)
				{
					stream.ReadUInt8(tempoByte);
					t = tempoByte << 8 * i;
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
			case 0x9: // Note On
			{
				Uint8 note = 0;
				stream.ReadUInt8(note);

				Uint8 velocity = 0;
				stream.ReadUInt8(velocity);
				if (velocity == 0) // If velocity is 0, then the sequence means to do "Note Off"
				{
					MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_NOTE_OFF, delta, channel, note, velocity));
				}
				else
				{
					MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_NOTE_ON, delta, channel, note, velocity));
				}
			}
			break;
			case 0x8: // Note Off
			{
				Uint8 note = 0;
				stream.ReadUInt8(note);
				Uint8 velocity = 0;
				stream.ReadUInt8(velocity);

				MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_NOTE_OFF, delta, channel, note, velocity));
			}
			break;
			case 0xc: // Program Change
			{
				Uint8 prog = 0;
				stream.ReadUInt8(prog);
				MessageList.push_back(AliveAudioMidiMessage(ALIVE_MIDI_PROGRAM_CHANGE, delta, channel, 0, 0, prog));
			}
			break;
			case 0xa: // Polyphonic key pressure (after touch)
			{
				Uint8 note = 0;
				Uint8 pressure = 0;

				stream.ReadUInt8(note);
				stream.ReadUInt8(pressure);
			}
			break;
			case 0xb: // Controller Change
			{
				Uint8 controller = 0;
				Uint8 value = 0;
				stream.ReadUInt8(controller);
				stream.ReadUInt8(value);
			}
			break;
			case 0xd: // After touch
			{
				Uint8 value = 0;
				stream.ReadUInt8(value);
			}
			break;
			case 0xe: // Pitch Bend
			{
				Uint16 bend = 0;
				stream.ReadUInt16(bend);
			}
			break;
			case 0xf: // Sysex len
			{
				const Uint32 length = midi_read_var_len(stream);
				snd_midi_skip_length(stream, length);
			}
			break;
			default:
				throw std::runtime_error("Unknown MIDI command");
			}

		}
	}
}