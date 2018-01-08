#include "cputimer.h"

// inv_freq is 1 over the number of ticks per second.
static double inv_freq;
static int freq_initialized = 0;
static int use_high_res_timer = 0;

std::chrono::time_point<std::chrono::high_resolution_clock> currentTime()
{
	return std::chrono::high_resolution_clock::now();
}
