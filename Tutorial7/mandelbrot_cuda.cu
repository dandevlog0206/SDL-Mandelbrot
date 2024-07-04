#include "mandelbrot_cuda.h"

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include "color.h"

using namespace std;

using real_t    = Mandelbrot::real_t;
using PixelInfo = Mandelbrot::PixelInfo;

template <class T>
__device__ inline uint32_t mandelbrot(T& cx, T& cy, uint32_t max_iter);
__device__ inline uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
__device__ inline uint32_t lerp_color(uint32_t a, uint32_t b, double t);

__device__ inline void acc_color(uint32_t col, uint32_t& r, uint32_t& g, uint32_t& b) {
	struct Color { uint8_t b, g, r, a; } color = *(Color*)&col;

	r += color.r;
	g += color.g;
	b += color.b;
}

static __device__ __forceinline__ uint32_t lcg(uint32_t& prev)
{
	prev = (1664525u * prev + 1013904223u);
	return prev & 0x00FFFFFF;
}

static __device__ __forceinline__ float rnd(uint32_t& prev)
{
	return ((float)lcg(prev) / (float)0x01000000);
}

struct Constants {
	int      width;
	int      height;
	uint32_t iter;

	uint32_t color_idx;
	real_t   color_scale;
	bool     color_smooth;

	bool     stop_all;
};

extern "C" {
	__constant__ uint32_t cuda_colormap[6][256];
	__constant__  Constants params;
}

template <class T>
__device__ uint32_t get_color_idx(T iter) {
	return (uint32_t)(params.color_scale * 256 * iter / params.iter) % 256;
}

__global__ void mandelbrot_kernel(
	uint32_t* device_surface,
	PixelInfo* device_pixel_info,
	uint32_t   sample_total,
	uint32_t   sample,
	real_t     min_x,
	real_t     max_y,
	real_t     dp
) {
	if (params.stop_all) return;

	int w = blockDim.x * blockIdx.x + threadIdx.x;
	int h = blockDim.x * blockIdx.y + threadIdx.y;

	if (params.width <= w || params.height <= h) return;

	auto pixel_off  = h * params.width + w;
	auto& info      = *(device_pixel_info + pixel_off);

	if (info.sample_count >= sample_total) return;
 
	auto seed  = (uint32_t)clock();
	auto count = min(sample_total - info.sample_count, sample);

	uint32_t col;
	uint32_t r = info.acc_r;
	uint32_t g = info.acc_g;
	uint32_t b = info.acc_b;

	for (uint32_t i = 0; i < count; ++i) {
		if (params.stop_all) return;
		
		real_t cx = min_x + dp * (w + rnd(seed));
		real_t cy = max_y - dp * (h + rnd(seed));

		auto iterated = mandelbrot<real_t>(cx, cy, params.iter);

		if (iterated == params.iter) {
			col = 0xff000000;
		} else if (!params.color_smooth) {
			col = cuda_colormap[params.color_idx][get_color_idx(iterated)];
		} else {
			double log_zn  = log(cx * cx + cy * cy) / 2.;
			double nu      = log(log_zn / log(2.)) / log(2.);
			auto real_iter = iterated + 3.5 - nu;

			auto col1 = cuda_colormap[params.color_idx][get_color_idx(real_iter)];
			auto col2 = cuda_colormap[params.color_idx][get_color_idx(real_iter + 1.)];

			col = lerp_color(col1, col2, fmod(real_iter, 1.));
		}

		acc_color(col, r, g, b);
	}

	info.sample_count += count;
	info.acc_r         = r;
	info.acc_g         = g;
	info.acc_b         = b;

	auto& pixel = *(device_surface + pixel_off);
	pixel = make_color(r / info.sample_count, g / info.sample_count, b / info.sample_count);
}

MandelbrotCUDA::MandelbrotCUDA(SDL_Renderer* renderer) 
	: Mandelbrot(renderer)
{
	size_t size = surface->w * surface->h;

	cudaHostRegister(surface->pixels, size * sizeof(uint32_t), cudaHostRegisterDefault);
	cudaHostRegister(render_info.pixels, size * sizeof(PixelInfo), cudaHostRegisterDefault);

	cudaMalloc((void**)&device_surface, size * sizeof(uint32_t));
	cudaMalloc((void**)&device_pixel_info, size * sizeof(PixelInfo));

	cudaMemcpyToSymbol(cuda_colormap, colormap, sizeof(colormap));

	cudaStreamCreate(&streams[0]);
	cudaStreamCreate(&streams[1]);

	block_size        = 8;
	sample_total      = 1;
	sample_per_launch = 1;
	sample_count      = 0;

	update();
}

MandelbrotCUDA::~MandelbrotCUDA()
{
	cudaHostUnregister(surface->pixels);
	cudaHostUnregister(render_info.pixels);
	cudaFree(device_surface);
	cudaFree(device_pixel_info);
	cudaStreamDestroy(streams[0]);
	cudaStreamDestroy(streams[1]);
}

void MandelbrotCUDA::setTotalSample(uint32_t sample)
{
	auto clear = sample_count > (sample_total = sample);
	update(clear, clear);
}

void MandelbrotCUDA::draw()
{
	size_t size = surface->w * surface->h * sizeof(uint32_t);

	cudaMemcpyAsync(surface->pixels, device_surface, size, cudaMemcpyDeviceToHost, streams[0]);
	cudaStreamSynchronize(streams[0]);
	Mandelbrot::draw();
}

void MandelbrotCUDA::stop()
{
	if (is_rendering) {
		size_t offset = offsetof(Constants, stop_all);

		cudaMemcpyToSymbolAsync(params, &is_rendering, 1, offset, cudaMemcpyHostToDevice, streams[0]);
		wait();
		cudaMemcpyToSymbolAsync(params, &is_rendering, 1, offset, cudaMemcpyHostToDevice, streams[0]);
	}
}

void MandelbrotCUDA::wait()
{
	cudaStreamSynchronize(streams[1]);
}

void MandelbrotCUDA::resize()
{
	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	
	size_t size = width * height;

	stop();
	cudaFree(device_surface);
	cudaFree(device_pixel_info);

	cudaMalloc((void**)&device_surface, size * sizeof(uint32_t));
	cudaMalloc((void**)&device_pixel_info, size * sizeof(PixelInfo));
	
	cudaHostUnregister(surface->pixels);
	cudaHostUnregister(render_info.pixels);

	Mandelbrot::resize();

	cudaHostRegister(surface->pixels, size * sizeof(uint32_t), cudaHostRegisterDefault);
	cudaHostRegister(render_info.pixels, size * sizeof(PixelInfo), cudaHostRegisterDefault);
}

void MandelbrotCUDA::move(int32_t rel_px, int32_t rel_py)
{
	stop();

	size_t size = surface->w * surface->h;

	cudaMemcpy(surface->pixels, device_surface, size * sizeof(uint32_t), cudaMemcpyDeviceToHost);
	cudaMemcpy(render_info.pixels, device_pixel_info, size * sizeof(PixelInfo), cudaMemcpyDeviceToHost);

	Mandelbrot::move(rel_px, rel_py);
	
	cudaMemcpy(device_surface, surface->pixels, size * sizeof(uint32_t), cudaMemcpyHostToDevice);
	cudaMemcpy(device_pixel_info, render_info.pixels, size * sizeof(PixelInfo), cudaMemcpyHostToDevice);
}

void MandelbrotCUDA::drawSurface()
{
	dim3 grid((width - 1) / block_size + 1, (height - 1) / block_size + 1);
	dim3 block(block_size, block_size);

	real_t min_x = pos_x - 2. * scale * aspect;
	real_t max_y = pos_y + 2. * scale;
	real_t dp    = 4. * scale / height;

	while (sample_count < sample_total) {
		if (stop_all) break;

		mandelbrot_kernel<<<grid, block, 0, streams[1]>>> (
			device_surface,
			device_pixel_info,
			sample_total,
			sample_per_launch,
			min_x, max_y, dp);

		sample_count = min(sample_count + sample_per_launch, sample_total);
		cudaStreamSynchronize(streams[1]);
	}

	cudaStreamSynchronize(streams[1]);
}

void MandelbrotCUDA::update(bool rerender_all, bool clear_surface)
{
	Mandelbrot::update(rerender_all, clear_surface);
	
	sample_count = 0;
	
	size_t size = surface->w * surface->h;

	if (rerender_all)
		cudaMemset(device_pixel_info, 0, size * sizeof(PixelInfo));
	if (clear_surface)
		cudaMemset(device_surface, 0, size * sizeof(uint32_t));

	Constants constants = {
		width, height, iter, color_idx,color_scale, smooth, stop_all
	};

	cudaMemcpyToSymbol(params, &constants, sizeof(Constants));
}