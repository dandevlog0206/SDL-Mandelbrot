#define SDL_MAIN_HANDLED
#define INITIAL_WIDTH  1280
#define INITIAL_HEIGHT 720
#define FRAME_LIMIT 60

#include <iostream>
#include <memory>
#include <SDL2/SDL.h>

#define TIME_IMPL
#include "mandelbrot_tbb.h"
#include "mandelbrot_cuda.h"
#include "time.h"
#include "gui.h"

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

void EventAsync(unique_ptr<GUI>& gui, unique_ptr<Mandelbrot>& mandelbrot) {
	if (gui->keyCaptured()) return;
	auto* keyStates = SDL_GetKeyboardState(NULL);
	if (keyStates[SDL_SCANCODE_W] || keyStates[SDL_SCANCODE_UP])
		mandelbrot->move(0, gui->settings.move_speed * Time::dt);
	if (keyStates[SDL_SCANCODE_A] || keyStates[SDL_SCANCODE_LEFT])
		mandelbrot->move(gui->settings.move_speed * Time::dt, 0);
	if (keyStates[SDL_SCANCODE_S] || keyStates[SDL_SCANCODE_DOWN])
		mandelbrot->move(0, -gui->settings.move_speed * Time::dt);
	if (keyStates[SDL_SCANCODE_D] || keyStates[SDL_SCANCODE_RIGHT])
		mandelbrot->move(-gui->settings.move_speed * Time::dt, 0);
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

bool EventProc(unique_ptr<GUI>& gui, unique_ptr<Mandelbrot>& mandelbrot) {
	static bool mouse_pressed = false;

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		gui->processEvent(&e);
		switch (e.type) {
		case SDL_QUIT: return true;
		case SDL_KEYDOWN:
			if (gui->keyCaptured()) break;
			KeyProc(e.key, mandelbrot);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT)
				mouse_pressed = e.button.state;
			break;
		case SDL_MOUSEMOTION:
			if (gui->mouseCaptured()) break;
			if (mouse_pressed)
				mandelbrot->move(e.motion.xrel, e.motion.yrel);
			break;
		case SDL_MOUSEWHEEL:
			if (gui->mouseCaptured()) break;
			if (e.wheel.y != 0) {
				auto scale = mandelbrot->getScale();
				if (e.wheel.y > 0) scale *= gui->settings.scroll_scale;
				else scale /= gui->settings.scroll_scale;

				if (gui->settings.scaleToCursor) {
					int px, py;
					SDL_GetMouseState(&px, &py);
					mandelbrot->setScaleTo(scale, px, py);
				} else {
					mandelbrot->setScale(scale);
				}
			}
			break;
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_RESIZED)
				mandelbrot->resize();
		}
	}

	EventAsync(gui, mandelbrot);
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
		SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	unique_ptr<Mandelbrot> mandelbrot = make_unique<MandelbrotCUDA>(renderer);
	unique_ptr<GUI>        gui        = make_unique<GUI>(renderer);

	bool closed = false;
	while (!closed) {
		closed = EventProc(gui, mandelbrot);

		mandelbrot->render(gui->settings.render_async); // async
		gui->update(mandelbrot);
		gui->render();

		if (gui->settings.reset_params) {
			mandelbrot->setPosition(0., 0.);
			mandelbrot->setScale(1.);
			mandelbrot->setIteration(32);
			gui->settings.reset_params = false;
		}

		mandelbrot->draw();
		gui->draw();
		SDL_RenderPresent(renderer);

		Time::update();
	}

	mandelbrot.reset();

	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	Terminate();
	return 0;
}