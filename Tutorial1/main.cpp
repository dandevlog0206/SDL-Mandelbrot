#define SDL_MAIN_HANDLED
#define INITIAL_WIDTH  680
#define INITIAL_HEIGHT 480

#include <iostream>
#include <thread>
#include <memory>
#include <SDL2/SDL.h>

#include "mandelbrot.h"

using namespace std;

void Init() {
	if (SDL_Init(SDL_INIT_VIDEO)) {
		cout << "error initializing SDL: " << SDL_GetError() << endl;
		exit(-1);
	}
}

void Terminate() {
	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Init();

	SDL_Window* window = SDL_CreateWindow(
		"mandelbrot sample",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		INITIAL_WIDTH,
		INITIAL_HEIGHT,
		NULL);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	auto mandelbrot = make_unique<Mandelbrot>(renderer);

	bool closed = false;
	while (!closed) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT: closed = true; break;
			}
		}

		mandelbrot->draw();
		SDL_RenderPresent(renderer);

		this_thread::sleep_for(30ms);
	}

	mandelbrot.reset();

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	Terminate();
	return 0;
}