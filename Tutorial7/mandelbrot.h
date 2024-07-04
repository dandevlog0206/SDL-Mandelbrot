#pragma once

#include <complex>
#include <future>
#include <atomic>
#include <SDL2/SDL.h>

template <class T>
inline uint32_t mandelbrot(T& cx, T& cy, uint32_t max_iter) {
	T cx_ = cx, cy_ = cy, zx = 0., zy = 0.;
	int i = 0;

	do {
		T temp = zx * zx - zy * zy + cx_;
		zy = 2. * zx * zy + cy_;
		zx = temp;
	} while (zx * zx + zy * zy < 65536. && ++i < max_iter);

	cx = zx;
	cy = zy;

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

	virtual void resize();

	std::complex<real_t> getPosition() const { return { pos_x, pos_y }; }
	void setPosition(real_t x, real_t y);
	virtual void move(int32_t rel_px, int32_t rel_py);

	inline real_t getScale() const { return scale; };
	void setScale(real_t scale);
	void setScaleTo(real_t scale, real_t px, real_t py);

	inline uint32_t getIteration() const { return iter; }
	void setIteration(uint32_t iter);

	inline uint32_t getColormap() const { return color_idx; }
	void setColormap(uint32_t idx);

	inline real_t getColorScale() const { return color_scale; }
	void setColorScale(real_t scale);

	inline bool getColorSmooth() const { return smooth; }
	void setColorSmooth(bool val);

	std::complex<real_t> pixelToComplex(real_t px, real_t py) const;

	const SDL_Surface* getSurface() const { return surface; }

protected:
	virtual void startAsync();
	virtual void drawSurface();
	virtual void update(bool rerender_all = true, bool clear_surface = true);

public:
	struct PixelInfo {
		union {
			struct {
				uint32_t sample_count;
				uint32_t acc_r;
				uint32_t acc_g;
				uint32_t acc_b;
			};
			bool rendered = false;
		};
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

	uint32_t color_idx;
	real_t   color_scale;
	bool     smooth;

	std::future<void> future;
	std::atomic<bool> is_rendering;
	std::atomic<bool> stop_all;

	bool updated;
};