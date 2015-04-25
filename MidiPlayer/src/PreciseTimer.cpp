#include "PreciseTimer.h"

void mgPreciseTimer::Start()
{
	QueryPerformanceFrequency(&perfFreq);
	QueryPerformanceCounter(&startTime);
}

void mgPreciseTimer::End()
{
	QueryPerformanceCounter(&endTime);
}

double mgPreciseTimer::GetDuration()
{
	QueryPerformanceCounter(&endTime);
	return (endTime.QuadPart - startTime.QuadPart) /
		(double)perfFreq.QuadPart;
}