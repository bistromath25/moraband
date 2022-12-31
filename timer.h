#ifndef TIMER_H
#define TIMER_H

#include <chrono>
//#include <sys/time.h>

const int MIN_SEARCH_TIME = 500; // Absolute minimum time to spend searching
const int MOVE_OVERHEAD = 500;

class Clock {
public:
	void set() {
		mTime = std::chrono::high_resolution_clock::now();
	}
	template<typename T> int64_t elapsed() {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<T>(now - mTime).count();
	}
private:
	std::chrono::high_resolution_clock::time_point mTime;
};

// Bob Hyatt's time management formula (with increment)
inline int64_t allocate_time(int time_left, int inc, int moves, int moves_to_go) {
	int64_t num_moves, target, search_time;
	float factor;
	num_moves = std::min(moves, 10);
	factor = 2 - num_moves / 10.0;
	target = std::max((time_left - MOVE_OVERHEAD) / (moves_to_go ? moves_to_go : 27), MIN_SEARCH_TIME);
	search_time = factor * target + inc / 2;
	return search_time > MIN_SEARCH_TIME ? search_time : MIN_SEARCH_TIME;
}

#endif

///