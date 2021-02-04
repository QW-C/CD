#pragma once

#include <CD/Common/Common.hpp>
#include <windows.h>

namespace CD {

class Clock {
public:
	Clock();

	void reset();

	double get_elapsed_time_ms();
private:
	LARGE_INTEGER start;
	double frequency_inv;
};

}