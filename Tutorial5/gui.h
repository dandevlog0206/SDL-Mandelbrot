#pragma once

#include <string>
#include <memory>
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

	void update(std::unique_ptr<Mandelbrot>& mandelbrot);
	void render();
	void draw();

	void saveCapture(const SDL_Surface* surface);

	struct {
		Acc     accelerator;
		float   move_speed;
		float   scroll_scale;
		bool    render_async;
		bool    scaleToCursor;
		bool    reset_params;
		bool    auto_iter;
		int32_t initial_iter;
		char    capture_dir[100];
		bool    capture_no_ui;
	} settings;

	void postErrorMessage(const char* msg);

private:
	void acceleratorChanged(std::unique_ptr<Mandelbrot>& mandelbrot);

private:
	SDL_Renderer* renderer;
	SDL_Window*   window;

	std::string message;
	int         msg_timepoint;
};