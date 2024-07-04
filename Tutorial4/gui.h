#pragma once

#include <string>
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
	bool keyCaptured() const;

	void render(const Mandelbrot* mandelbrot);
	void draw();

	void saveCapture(const SDL_Surface* surface);

	struct {
		Acc   accelerator;
		float move_speed;
		float scroll_scale;
		bool  scaleToCursor;
		bool  reset_params;
		char  capture_dir[100];
		bool  capture_no_ui;
	} settings;

	void postErrorMessage(const char* msg);

private:
	SDL_Renderer* renderer;
	SDL_Window*   window;

	std::string message;
	int         msg_timepoint;
};