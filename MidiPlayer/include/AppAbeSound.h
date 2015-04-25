#pragma once
#include "Application.h"
#include "SoundSystem.hpp"

class AppAbeSound :
	public mgApplication
{
public:
	int Start();
	int Shutdown();

	int Update();
	int Draw();
};

