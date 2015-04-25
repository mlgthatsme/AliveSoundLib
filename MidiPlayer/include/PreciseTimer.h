#pragma once

#include <Windows.h>

static class mgPreciseTimer
{
public:
	LARGE_INTEGER perfFreq, startTime, endTime;
	void Start();
	void End();
	double GetDuration();
private:
};

