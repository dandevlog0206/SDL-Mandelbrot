#include "mandelbrot.h"

#include <complex>

using namespace std;

template <class T>
inline uint32_t mandelbrot(complex<T> c, uint32_t max_iter) {
	complex<T> z;
	for (int i = 0; i < max_iter; ++i) {
		z = z * z + c;
		if (norm(z) > 4.) return i;
	}
	return max_iter;
}

inline uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff) {
	return (a << 24) + (r << 16) + (g << 8) + b;
}

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
	iter  = 10;

	updated = false;
}

Mandelbrot::~Mandelbrot()
{
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void Mandelbrot::draw()
{
	if (!updated) {
		drawSurface();
		SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
		updated = true;
	}

	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
}

void Mandelbrot::drawSurface()
{
	real_t min_x = pos_x - 2. * scale * aspect;
	real_t max_y = pos_y + 2. * scale;
	real_t dx    = 4. * scale * aspect / width;
	real_t dy    = 4. * scale / height;

	for (int h = 0; h < height; ++h) {
		for (int w = 0; w < width; ++w) {
			uint32_t& pixel = *((uint32_t*)surface->pixels + h * surface->w + w);

			real_t cx = min_x + dx * (w + 0.5f);
			real_t cy = max_y - dy * (h + 0.5f);

			auto iterated = mandelbrot<real_t>({ cx, cy }, iter);

			uint8_t col = 255.99f * iterated / iter;
			pixel       = make_color(col, col, col);
		}
	}
}