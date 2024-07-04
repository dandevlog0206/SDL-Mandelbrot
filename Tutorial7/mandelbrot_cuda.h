#pragma once

#include "mandelbrot.h"

struct CUstream_st;
typedef struct CUstream_st* cudaStream_t;

class MandelbrotCUDA : public Mandelbrot
{
public:
	using real_t = Mandelbrot::real_t;

	using Mandelbrot::Mandelbrot;

	MandelbrotCUDA(SDL_Renderer* renderer);
	~MandelbrotCUDA() override;

	inline uint32_t getBlockSize() const { return block_size; }
	inline void setBlockSize(uint32_t size) { block_size = size; }

	inline uint32_t getTotalSample() const { return sample_total; }
	void setTotalSample(uint32_t sample);

	inline uint32_t getSamplePerLaunch() const { return sample_per_launch; }
	inline void setSamplePerLaunch(uint32_t sample) { sample_per_launch = sample; }

	inline uint32_t getSampleCount() const { return sample_count; }

	void draw() override;
	void stop() override;
	void wait() override;
	void resize() override;

private:
	void move(int32_t rel_px, int32_t rel_py) override;
	void drawSurface() override;
	void update(bool rerender_all = true, bool clear_surface = true) override;

	uint32_t*  device_surface;
	PixelInfo* device_pixel_info;

	uint32_t block_size;
	uint32_t sample_total;
	uint32_t sample_per_launch;
	uint32_t sample_count;

	cudaStream_t streams[2];
};