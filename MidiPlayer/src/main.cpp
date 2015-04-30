//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include "AppAbeSound.h"
#include "SoundSystem.hpp"
#include "FileManager.h"
#include "json\jsonxx.h"

int main(int argc, char* args[])
{
	std::string jsonText = mgFileManager::ReadFileToString("data/json/music.json");

	jsonxx::Object obj;
	obj.parse(jsonText);

	jsonxx::Array levelList = obj.get<jsonxx::Array>("themes");

	for (int i = 0; i < levelList.size(); i++)
	{
		std::string s = levelList.get<jsonxx::Object>(i).get<jsonxx::String>("lvl");
		printf("%s\n", s.c_str());
	}

	

	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;

	//The window we'll be rendering to
	SDL_Window* window = NULL;

	//The surface contained by the window
	SDL_Surface* screenSurface = NULL;

	AliveInitAudio();

	AppAbeSound * app = new AppAbeSound();
	app->Run();
	delete app;

	SDL_Quit();

	return 0;
}