#pragma once

#include <SDL2/SDL.h>

class Mandelbrot
{
public:
	using real_t = double;

	Mandelbrot(SDL_Renderer* renderer);
	~Mandelbrot();

	void draw();

private:
	void drawSurface();

private:
	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_Surface* surface;
	SDL_Texture* texture;

	int width;
	int height;
	real_t aspect;

	real_t pos_x;
	real_t pos_y;
	real_t scale;
	uint32_t iter;

	bool updated;
};