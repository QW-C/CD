#include <CD/Common/Clock.hpp>
#include <profileapi.h>

namespace CD {

Clock::Clock() {
	LARGE_INTEGER frequency;
	::QueryPerformanceFrequency(&frequency);
	frequency_inv = 1000. / frequency.QuadPart;
	reset();
}

void Clock::reset() {
	::QueryPerformanceCounter(&start);
}

double Clock::get_elapsed_time_ms() {
	LARGE_INTEGER ticks;
	::QueryPerformanceCounter(&ticks);
	return (ticks.QuadPart - start.QuadPart) * frequency_inv;
}

}