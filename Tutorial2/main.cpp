#define SDL_MAIN_HANDLED
#define INITIAL_WIDTH  680
#define INITIAL_HEIGHT 480
#define FRAME_LIMIT 60

#include <iostream>
#include <memory>
#include <SDL2/SDL.h>

#define TIME_IMPL
#include "mandelbrot.h"
#include "time.h"

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

void EventAsync(unique_ptr<Mandelbrot>& mandelbrot) {
	auto* keyStates = SDL_GetKeyboardState(NULL);
	if (keyStates[SDL_SCANCODE_W] || keyStates[SDL_SCANCODE_UP])
		mandelbrot->move(0, 100 * Time::dt);
	if (keyStates[SDL_SCANCODE_A] || keyStates[SDL_SCANCODE_LEFT])
		mandelbrot->move(100 * Time::dt, 0);
	if (keyStates[SDL_SCANCODE_S] || keyStates[SDL_SCANCODE_DOWN])
		mandelbrot->move(0, -100 * Time::dt);
	if (keyStates[SDL_SCANCODE_D] || keyStates[SDL_SCANCODE_RIGHT])
		mandelbrot->move(-100 * Time::dt, 0);
}

void KeyProc(SDL_KeyboardEvent& e, unique_ptr<Mandelbrot>& mandelbrot) {
	switch (e.keysym.scancode) {
	case SDL_SCANCODE_COMMA: {
		auto iter = mandelbrot->getIteration();
		if (iter > 1)
			mandelbrot->setIteration(iter - 1);
		break;
	}
	case SDL_SCANCODE_PERIOD:
		auto iter = mandelbrot->getIteration();
		mandelbrot->setIteration(iter + 1);
		break;
	}
}

bool EventProc(unique_ptr<Mandelbrot>& mandelbrot) {
	static bool mouse_pressed = false;

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT: return true;
		case SDL_KEYDOWN:
			KeyProc(e.key, mandelbrot);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT)
				mouse_pressed = e.button.state;
			break;
		case SDL_MOUSEMOTION:
			if (mouse_pressed)
				mandelbrot->move(e.motion.xrel, e.motion.yrel);
			break;
		case SDL_MOUSEWHEEL:
			if (e.wheel.y != 0) {
				int px, py;
				SDL_GetMouseState(&px, &py);
				auto scale = mandelbrot->getScale();
				scale     *= e.wheel.y > 0 ? 1.1 : 1. / 1.1;
				mandelbrot->setScaleTo(scale, px, py);
			}
			break;
		}
	}

	EventAsync(mandelbrot);
	return false;
}

int main(int argc, char* argv[]) {
	Time::fps_limit = FRAME_LIMIT;
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
		closed = EventProc(mandelbrot);

		mandelbrot->draw();
		SDL_RenderPresent(renderer);

		Time::update();
		cout << Time::fps << "  " << Time::dt << endl;
	}

	mandelbrot.reset();

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	Terminate();
	return 0;
}