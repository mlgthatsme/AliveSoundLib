//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include "AppAbeSound.h"
#include "SoundSystem.hpp"

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

	//Initialize SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	AliveInitAudio();

	AppAbeSound app;
	app.Run();

	SDL_Quit();

	return 0;
}