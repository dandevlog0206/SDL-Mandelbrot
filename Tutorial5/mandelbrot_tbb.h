#pragma once

#include "mandelbrot.h"

class MandelbrotTBB : public Mandelbrot
{
public:
	using real_t = Mandelbrot::real_t;

	MandelbrotTBB(SDL_Renderer* renderer);
	~MandelbrotTBB() override;

private:
	void drawSurface() override;
};