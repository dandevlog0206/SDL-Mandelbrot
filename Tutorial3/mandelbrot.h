#pragma once

#include <complex>
#include <SDL2/SDL.h>

class Mandelbrot
{
public:
	using real_t = double;

	Mandelbrot(SDL_Renderer* renderer);
	~Mandelbrot();

	void draw();

	void resize();

	std::complex<real_t> getPosition() const { return { pos_x, pos_y }; }
	void setPosition(real_t x, real_t y);
	void move(int32_t rel_px, int32_t rel_py);

	inline real_t getScale() const { return scale; };
	void setScale(real_t scale);
	void setScaleTo(real_t scale, real_t px, real_t py);

	inline uint32_t getIteration() const { return iter; }
	void setIteration(uint32_t iter);

	std::complex<real_t> pixelToComplex(real_t px, real_t py) const;

private:
	void drawSurface();

private:
	SDL_Window*   window;
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