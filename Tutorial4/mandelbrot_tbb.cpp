#include "mandelbrot_tbb.h"

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/blocked_range2d.h>
#include <oneapi/tbb/task.h>

using namespace std;
using namespace oneapi;

MandelbrotTBB::MandelbrotTBB(SDL_Renderer* renderer)
	: Mandelbrot(renderer) 
{
}

MandelbrotTBB::~MandelbrotTBB()
{
}

void MandelbrotTBB::drawSurface()
{
	using range_t = tbb::blocked_range2d<int, int>;

	real_t min_x   = pos_x - 2. * scale * aspect;
	real_t max_y   = pos_y + 2. * scale;
	real_t dx      = 4. * scale * aspect / width;
	real_t dy      = 4. * scale / height;

	tbb::parallel_for(range_t(0, height, 0, width), [=](range_t& r) {
		for (int h = r.rows().begin(); h < r.rows().end(); ++h) {
			if (stop_all) {
				tbb::task::current_context()->cancel_group_execution();
				return;
			}

			for (int w = r.cols().begin(); w < r.cols().end(); ++w) {
				uint32_t& pixel = *((uint32_t*)surface->pixels + h * surface->w + w);

				real_t cx = min_x + dx * (w + 0.5f);
				real_t cy = max_y - dy * (h + 0.5f);

				auto iterated = mandelbrot<real_t>(cx, cy, iter);

				uint8_t col = 255.99f * (float)iterated / iter;
				pixel = make_color(col, col, col);
			}
		}
	});
}