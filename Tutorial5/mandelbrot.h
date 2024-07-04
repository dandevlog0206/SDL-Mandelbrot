#pragma once

#include <complex>
#include <future>
#include <atomic>
#include <SDL2/SDL.h>

inline uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff) {
	return (a << 24) + (r << 16) + (g << 8) + b;
}

template <class T>
inline uint32_t mandelbrot(T cx, T cy, uint32_t max_iter) {
	T zx = 0., zy = 0.;
	int i = 0;

	do {
		T temp = zx * zx - zy * zy + cx;
		zy = 2. * zx * zy + cy;
		zx = temp;
	} while (zx * zx + zy * zy < 4. && ++i < max_iter);

	return i;
}

class Mandelbrot
{
public:
	using real_t = double;

	Mandelbrot(SDL_Renderer* renderer);
	virtual ~Mandelbrot();

	void render(bool async = true);
	virtual void draw();

	virtual void stop();
	virtual void wait();
	virtual bool isRendering() const;

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

	const SDL_Surface* getSurface() const { return surface; }

protected:
	virtual void startAsync();
	virtual void drawSurface();
	void update(bool rerender_all = true, bool clear_surface = true);

public:
	struct PixelInfo {
		bool rendered = false;
	};

	struct RenderInfo {
		void resize(uint32_t width, uint32_t height);
		void reset(PixelInfo info = {});
		void move(int32_t rel_px, int32_t rel_py);
		void fillRect(SDL_Rect* rect, PixelInfo info = {});
		void destroy();

		inline PixelInfo& at(uint32_t px, uint32_t py) {
			return *(pixels + py * width + px);
		};

		PixelInfo* pixels;
		uint32_t   width;
		uint32_t   height;
	};

protected:
	SDL_Window*   window;
	SDL_Renderer* renderer;

	RenderInfo   render_info;
	SDL_Surface* surface_temp;
	SDL_Surface* surface;
	SDL_Texture* texture;

	int width;
	int height;
	real_t aspect;

	real_t pos_x;
	real_t pos_y;
	real_t scale;
	uint32_t iter;

	std::future<void> future;
	std::atomic<bool> is_rendering;
	std::atomic<bool> stop_all;

	bool updated;
};