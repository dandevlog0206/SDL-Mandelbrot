#pragma once

#include <SDL2/SDL.h>

class Mandelbrot;

class GUI
{
	enum class Acc {
		CPU     = 0,
		CPU_TBB = 1,
	};

public:
	GUI(SDL_Renderer* renderer);
	~GUI();

	void processEvent(SDL_Event* event);
	bool mouseCaptured();

	void render(const Mandelbrot* mandelbrot);
	void draw();

	struct {
		Acc   accelerator;
		float move_speed;
		float scroll_scale;
		bool  scaleToCursor;
		bool  reset_params;
	} settings;

private:
	SDL_Renderer* renderer;
	SDL_Window*   window;
};