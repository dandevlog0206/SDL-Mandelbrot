#pragma once

#include <thread>
#include <chrono>

class Time {
	Time() = delete;
	
public:
	using clock_t      = std::chrono::high_resolution_clock;
	using time_point_t = clock_t::time_point;
	using milliseconds = std::chrono::milliseconds;

	static inline void update() {
		double delta  = elapsed();
		int32_t delay = 1e3 * (1. / fps_limit - delta);

		if (delay > 0) {
			std::this_thread::sleep_for(milliseconds(delay));
			delta = elapsed();
		} 

		fps   = 1. / delta;
		dt    = delta;
		begin = end;
	}

	static uint32_t fps_limit;
	static double fps;
	static double dt;

private:
	static inline double elapsed() {
		end = clock_t::now();
		return (end - begin).count() / 1e9;
	}

	static time_point_t begin;
	static time_point_t end;
};

#ifdef TIME_IMPL
uint32_t Time::fps_limit = 60;
double Time::fps         = 0.;
double Time::dt          = 0.;

Time::time_point_t Time::begin = Time::clock_t::now();
Time::time_point_t Time::end   = Time::clock_t::now();
#endif