#pragma once

#include <oneapi/tbb/task_arena.h>

#include "mandelbrot.h"

class MandelbrotTBB : public Mandelbrot
{
public:
	using real_t = Mandelbrot::real_t;

	MandelbrotTBB(SDL_Renderer* renderer);
	~MandelbrotTBB() override;

	uint32_t getMaxConcurrency() const;
	void setMaxConcurrency(uint32_t val);

private:
	void startAsync() override;
	void drawSurface() override;

	oneapi::tbb::task_arena arena;
};