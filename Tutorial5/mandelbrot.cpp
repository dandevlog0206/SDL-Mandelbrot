#include "mandelbrot.h"

using namespace std;

using real_t = Mandelbrot::real_t;

Mandelbrot::Mandelbrot(SDL_Renderer* renderer)
	: renderer(renderer)
{
	window = SDL_RenderGetWindow(renderer);
	
	SDL_GetWindowSize(window, &width, &height);
	aspect = (real_t)width / height;

	render_info.resize(width, height);
	surface_temp = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	surface      = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	texture      = SDL_CreateTextureFromSurface(renderer, surface);

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
	render_info.destroy();
	SDL_FreeSurface(surface_temp);
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

	render_info.resize(width, height);
	surface_temp = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	surface      = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	texture      = SDL_CreateTextureFromSurface(renderer, surface);
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
	SDL_Rect rect1, rect2;
	SDL_Rect rect = { rel_px, rel_py, width + rel_px, height + rel_px };
	SDL_BlitSurface(surface, nullptr, surface, &rect);

	if (rel_px >= 0 && rel_py >= 0) {
		rect1 = { 0, 0, width, rel_py };
		rect2 = { 0, rel_py, rel_px, height - rel_py };
	} else if (rel_px >= 0 && rel_py <= 0) {
		rect1 = { 0, height + rel_py, width, -rel_py };
		rect2 = { 0, 0, rel_px, height + rel_py };
	} else if (rel_px <= 0 && rel_py >= 0) {
		rect1 = { 0, 0, width, rel_py };
		rect2 = { width + rel_px, rel_py, -rel_px, height - rel_py };
	} else if (rel_px <= 0 && rel_py <= 0) {
		rect1 = { 0, height + rel_py, width, -rel_py };
		rect2 = { width + rel_px, 0, -rel_px, height + rel_py };
	}

	render_info.move(rel_px, rel_py);
	render_info.fillRect(&rect1);
	render_info.fillRect(&rect2);
	SDL_FillRect(surface, &rect1, 0x00000000);
	SDL_FillRect(surface, &rect2, 0x00000000);

	real_t dx = 4. * scale * aspect / width;
	real_t dy = 4. * scale / height;

	pos_x  -= dx * rel_px;
	pos_y  += dy * rel_py;
	update(false, false);
}

void Mandelbrot::setScale(real_t scale) 
{
	stop();
	auto mag = this->scale / scale;
	int w    = width * (1. - mag);
	int h    = height * (1. - mag);

	SDL_Rect rect = { w / 2, h / 2, width - w, height - h };

	SDL_FillRect(surface_temp, nullptr, 0x00000000);
	SDL_BlitScaled(surface, nullptr, surface_temp, &rect);
	std::swap(surface, surface_temp);

	this->scale = scale;
	update(true, false);
}

void Mandelbrot::setScaleTo(real_t scale, real_t px, real_t py)
{
	stop();
	auto point  = pixelToComplex(px, py);
	auto mag    = this->scale / scale;
	int w       = width * (1. - mag);
	int h       = height * (1. - mag);

	SDL_Rect rect = { w * (px / width), h * (py / height), width - w, height - h};

	SDL_FillRect(surface_temp, nullptr, 0x00000000);
	SDL_BlitScaled(surface, nullptr, surface_temp, &rect);
	std::swap(surface, surface_temp);

	this->scale = scale;

	real_t dx = 4. * scale * aspect / width;
	real_t dy = 4. * scale / height;

	pos_x   = point.real() + 2. * scale * aspect - px * dx;
	pos_y   = point.imag() - 2. * scale + py * dy;
	update(true, false);
}

void Mandelbrot::setIteration(uint32_t iter)
{
	stop();
	this->iter = iter;
	update(true, false);
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
			auto& info = render_info.at(w, h);
			if (info.rendered) continue;

			uint32_t& pixel = *((uint32_t*)surface->pixels + h * surface->w + w);

			real_t cx = min_x + dx * (w + 0.5f);
			real_t cy = max_y - dy * (h + 0.5f);

			auto iterated = mandelbrot<real_t>(cx, cy, iter);

			uint8_t col   = 255.99f * iterated / iter;
			pixel         = make_color(col, col, col);
			info.rendered = true;
		}
	}
}

void Mandelbrot::update(bool rerender_all, bool clear_surface)
{
	stop();
	updated = false;

	if (rerender_all)
		render_info.reset();
	if (clear_surface)
		SDL_FillRect(surface, nullptr, 0x00000000);
}

void Mandelbrot::RenderInfo::resize(uint32_t width, uint32_t height)
{
	if (pixels) destroy();
	pixels       = new PixelInfo[width * height];
	this->width  = width;
	this->height = height;
}

void Mandelbrot::RenderInfo::reset(PixelInfo info)
{
	if (pixels) fill_n(pixels, width * height, info);
}

void Mandelbrot::RenderInfo::move(int32_t rel_px, int32_t rel_py)
{
	if (!pixels) return;

	if (rel_px >= 0 && rel_py >= 0) {
		for (int h = height - rel_py - 1; h >= 0; --h) 
			std::move_backward(&at(0, h), &at(width - rel_px, h), &at(width, h + rel_py));
	} else if (rel_px >= 0 && rel_py <= 0) {
		for (int h = -rel_py; h < height; ++h) 
			std::move_backward(&at(0, h), &at(width - rel_px, h), &at(width, h + rel_py));
	} else if (rel_px <= 0 && rel_py >= 0) {
		for (int h = height - rel_py - 1; h >= 0; --h) 
			std::move(&at(-rel_px, h), &at(width, h), &at(0, h + rel_py));
	} else if (rel_px <= 0 && rel_py <= 0) {
		for (int h = -rel_py; h < height; ++h) 
			std::move(&at(-rel_px, h), &at(width, h), &at(0, h + rel_py));
	}
}

void Mandelbrot::RenderInfo::fillRect(SDL_Rect* rect, PixelInfo info)
{
	if (!pixels) return;

	int h_min  = SDL_clamp(rect->y, 0, height);
	int h_max  = SDL_clamp(rect->y + rect->h, 0, height);
	int w_min  = SDL_clamp(rect->x, 0, width);
	int w_size = SDL_clamp(rect->x + rect->w, 0, width) - rect->x;

	for (int h = h_min; h < h_max; ++h)
		fill_n(&at(w_min, h), w_size, info);
}

void Mandelbrot::RenderInfo::destroy()
{
	delete pixels;
	pixels = nullptr;
}