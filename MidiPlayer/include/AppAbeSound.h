#pragma once
#include "Application.h"
#include "alive\AliveAudio.h"

class AppAbeSound :
	public mgApplication
{
public:
	int Start();
	int Shutdown();

	int Update();
	int Draw();
};

