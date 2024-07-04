#include "mandelbrot_tbb.h"

#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/blocked_range2d.h>
#include <oneapi/tbb/task.h>

#include "color.h"

using namespace std;
using namespace oneapi;

MandelbrotTBB::MandelbrotTBB(SDL_Renderer* renderer)
	: Mandelbrot(renderer) 
{
	arena.initialize();
}

MandelbrotTBB::~MandelbrotTBB()
{
	arena.terminate();
}

uint32_t MandelbrotTBB::getMaxConcurrency() const
{
	return arena.max_concurrency();
}

void MandelbrotTBB::setMaxConcurrency(uint32_t val)
{
	stop();
	arena.terminate();
	arena.initialize(val);
	update(false, false);
}

void MandelbrotTBB::startAsync() {
	promise<void> p;
	future = p.get_future();

	arena.enqueue([this, p = std::move(p)]() {
		is_rendering = true;
		drawSurface();
		is_rendering = false;

		const_cast<promise<void>&>(p).set_value();
	});
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
				auto& info = render_info.at(w, h);
				if (info.rendered) continue;

				uint32_t& pixel = *((uint32_t*)surface->pixels + h * surface->w + w);

				real_t cx = min_x + dx * (w + 0.5f);
				real_t cy = max_y - dy * (h + 0.5f);

				auto iterated = mandelbrot<real_t>(cx, cy, iter);

				if (iterated == iter) {
					pixel = 0xff000000;
				} else if (!smooth) {
					pixel = colormap[color_idx][(int)(color_scale * 256 * iterated / iter) % 256];
				}  else {
					double log_zn  = log(cx * cx + cy * cy) / 2.;
					double nu      = log(log_zn / log(2)) / log(2);
					auto real_iter = iterated + 3.5 - nu;
							
					auto col1 = colormap[color_idx][(int)(color_scale * 256 * real_iter / iter) % 256];
					auto col2 = colormap[color_idx][(int)(color_scale * 256 * (real_iter + 1) / iter) % 256];
							
					pixel = lerp_color(col1, col2, fmod(iter, 1));
				}

				info.rendered = true;
			}
		}
	});
}