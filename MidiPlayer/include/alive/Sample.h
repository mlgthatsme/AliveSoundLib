#pragma once

class AliveAudioSample
{
public:
	Uint16 * m_SampleBuffer;
	unsigned int i_SampleSize;

	float GetSample(float sampleOffset);
};