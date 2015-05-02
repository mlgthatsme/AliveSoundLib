//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include "AppAbeSound.h"
#include "FileManager.h"
#include "json\jsonxx.h"

int main(int argc, char* args[])
{
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