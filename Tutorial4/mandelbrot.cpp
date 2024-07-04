#include "mandelbrot.h"

using namespace std;

using real_t = Mandelbrot::real_t;

Mandelbrot::Mandelbrot(SDL_Renderer* renderer)
	: renderer(renderer)
{
	window = SDL_RenderGetWindow(renderer);
	
	SDL_GetWindowSize(window, &width, &height);
	aspect = (real_t)width / height;

	surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	texture = SDL_CreateTextureFromSurface(renderer, surface);

	pos_x = 0.;
	pos_y = 0.;
	scale = 1.;
	iter  = 100;

	is_rendering = false;
	stop_all     = false;
	updated      = false;
}

Mandelbrot::~Mandelbrot()
{
	stop();
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void Mandelbrot::render(bool async)
{
	if (!updated) {
		if (!async) {
			is_rendering = true; // just in case...
			drawSurface();
			is_rendering = false;
		} else 
			startAsync();

		updated = true;
	}
}

void Mandelbrot::draw()
{
	SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
}

void Mandelbrot::stop()
{
	if (is_rendering) {
		stop_all = true;
		wait();
		stop_all = false;
		is_rendering = false;
	}
}

void Mandelbrot::wait()
{
	if (is_rendering)
		future.get(); // or wait()
}

bool Mandelbrot::isRendering() const
{
	return is_rendering;
}

void Mandelbrot::resize()
{
	stop();
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);

	SDL_GetWindowSize(window, &width, &height);
	aspect = (real_t)width / height;

	surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	update();
}

void Mandelbrot::setPosition(real_t x, real_t y)
{
	stop();
	pos_x   = x;
	pos_y   = y;
	update();
}

void Mandelbrot::move(int32_t rel_px, int32_t rel_py)
{
	stop();
	real_t dx = 4. * scale * aspect / width;
	real_t dy = 4. * scale / height;

	pos_x  -= dx * rel_px;
	pos_y  += dy * rel_py;
	update();
}

void Mandelbrot::setScale(real_t scale) 
{
	stop();
	this->scale = scale;
	update();
}

void Mandelbrot::setScaleTo(real_t scale, real_t px, real_t py)
{
	stop();
	auto point  = pixelToComplex(px, py);
	this->scale = scale;

	real_t dx = 4. * scale * aspect / width;
	real_t dy = 4. * scale / height;

	pos_x   = point.real() + 2. * scale * aspect - px * dx;
	pos_y   = point.imag() - 2. * scale + py * dy;
	update();
}

void Mandelbrot::setIteration(uint32_t iter)
{
	stop();
	this->iter = iter;
	update();
}

std::complex<real_t> Mandelbrot::pixelToComplex(real_t px, real_t py) const
{
	real_t min_x = pos_x - 2. * scale * aspect;
	real_t max_y = pos_y + 2. * scale;
	real_t dx    = 4. * scale * aspect / width;
	real_t dy    = 4. * scale / height;
	return { min_x + px * dx, max_y - py * dy };
}

void Mandelbrot::startAsync()
{
	future = async(launch::async, [this] {
		is_rendering = true;
		drawSurface();
		is_rendering = false;
	});
}

void Mandelbrot::drawSurface()
{
	real_t min_x = pos_x - 2. * scale * aspect;
	real_t max_y = pos_y + 2. * scale;
	real_t dx    = 4. * scale * aspect / width;
	real_t dy    = 4. * scale / height;

	for (int h = 0; h < height; ++h) {
		if (stop_all) return;

		for (int w = 0; w < width; ++w) {
			uint32_t& pixel = *((uint32_t*)surface->pixels + h * surface->w + w);

			real_t cx = min_x + dx * (w + 0.5f);
			real_t cy = max_y - dy * (h + 0.5f);

			auto iterated = mandelbrot<real_t>(cx, cy, iter);

			uint8_t col = 255.99f * iterated / iter;
			pixel       = make_color(col, col, col);
		}
	}
}

void Mandelbrot::update()
{
	stop();
	updated = false;
}